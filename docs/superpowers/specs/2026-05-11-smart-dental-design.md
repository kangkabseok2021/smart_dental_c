# Smart Dental Handpiece — Design Spec

**Date:** 2026-05-11
**Goal:** GitHub portfolio project demonstrating MedTech skills — C++17, Qt 6, QML, multithreading, SQLite, CI/CD.
**Target platforms:** macOS + Linux (cross-platform CMake; Docker covers Linux headless path)

---

## 1. Architecture

Four-layer stack with Dependency Injection in `main.cpp`:

```
QML  (main.qml · Dashboard.qml)
        ▲  Q_PROPERTY / NOTIFY signals
TelemetryBridge          StateManager
        │  ▲                  │  ▲
        │  └── HardwareHandler┘  │
        │                        │
        └──── DatabaseManager ───┘
```

`HardwareHandler` emits only signals; it never writes to the DB. Both `TelemetryBridge` and `StateManager` hold a reference to `DatabaseManager` and write to it independently.

### Components

| Class | Responsibility |
|---|---|
| `HardwareHandler` | Worker-thread HAL. Polls simulated sensors at ~100ms. Emits RPM, temperature, usage hours, and fault signals. Never touches UI or DB. |
| `StateManager` | Owns the device state machine (C++ enum + transition table). Receives HardwareHandler signals, evaluates guards, drives transitions, writes events to DB. Exposed to QML as context property. |
| `TelemetryBridge` | Thin QObject. Subscribes to HardwareHandler sensor signals, re-exposes as `Q_PROPERTY` values for QML data binding. Owns a `QTimer` at 500ms that writes telemetry rows to DB while in `ACTIVE` state (timer started/stopped on `StateManager::stateChanged`). Exposed to QML as context property. |
| `DatabaseManager` | SQLite persistence via `QSqlDatabase`. Typed insert methods: `openSession()`, `closeSession()`, `logEvent()`, `logTelemetry()`. No business logic. |
| `main.cpp` | Constructs all objects, wires signals/slots, registers QML context properties, starts engine. The only file that knows all classes simultaneously. |

---

## 2. State Machine

Six states as `enum class DeviceState`:

```
STANDBY → CALIBRATING → READY ↔ ACTIVE
                  │                │
               [fail]       [fault / critical]
                  ↓                ↓
               FAULT ←─────── FAULT
                  │
           [3 faults / critical temp]
                  ↓
           EMERGENCY_STOP
                  │
          [manual clear + ack]
                  ↓
               STANDBY
```

**All states:**
- `STANDBY` — initial state; device idle, awaiting power-on
- `CALIBRATING` — startup self-check (3 consecutive valid sensor ticks required)
- `READY` — calibration passed; awaiting user start
- `ACTIVE` — running; sensors polled, telemetry logged
- `FAULT` — recoverable error; transitions back to STANDBY on `acknowledgeFault()`
- `EMERGENCY_STOP` — non-recoverable until `clearEmergency()` called explicitly

**Guard conditions:**

| Transition | Guard |
|---|---|
| `Active → Fault` | temp > 60°C **or** RPM dropout > 500ms **or** sensor NaN |
| `Active → EmergencyStop` | temp > 75°C **or** 3 consecutive faults in one session |
| `EmergencyStop → Standby` | `clearEmergency()` called (QML confirmation dialog required) |
| `Fault → Standby` | `acknowledgeFault()` called |
| `Calibrating → Ready` | 3 consecutive valid sensor ticks |
| `Calibrating → Fault` | any invalid sensor tick during calibration |

**Per-transition DB log:** `event_type`, `from_state`, `to_state`, detail string (e.g. `"temp=76.2°C"`), timestamp, session ID.

**Consecutive-fault counter:** `StateManager` maintains an in-memory `int m_sessionFaultCount` that increments on every `FAULT` entry and resets to 0 on `openSession()`. It is never read from the DB — the DB record is the audit trail, not the guard input.

---

## 3. Database Schema

File: `data/logs.db` (SQLite). Tables created via `CREATE TABLE IF NOT EXISTS` on first run.

```sql
CREATE TABLE sessions (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    started_at       TEXT NOT NULL,
    ended_at         TEXT,
    firmware_version TEXT NOT NULL
);

CREATE TABLE events (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id  INTEGER NOT NULL REFERENCES sessions(id),
    timestamp   TEXT NOT NULL,
    event_type  TEXT NOT NULL,   -- STATE_CHANGE | FAULT | EMERGENCY_STOP | FAULT_ACK | EMERGENCY_CLEAR
    from_state  TEXT,
    to_state    TEXT NOT NULL,
    detail      TEXT
);

CREATE TABLE telemetry (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id   INTEGER NOT NULL REFERENCES sessions(id),
    timestamp    TEXT NOT NULL,
    rpm          REAL NOT NULL,
    temperature  REAL NOT NULL,
    usage_hours  REAL NOT NULL
);
```

Telemetry rows inserted every 500ms while in `ACTIVE` state only.

---

## 4. QML Frontend

**Visual theme:** Dark Clinical — `#0d1117` background, `#4ade80` green vitals, `#f59e0b` amber warnings, `#ef4444` red for Emergency Stop.

**Layout:** Single page. `main.qml` is the window root and owns state-dependent visibility. `Dashboard.qml` is the content component.

**Page structure (top to bottom):**
1. **Status bar** — device name, current state badge (color-coded), firmware version
2. **Vitals row** — three metric cards: RPM, Temperature °C, Usage Hours
3. **RPM history chart** — Qt Charts `ChartView` with `LineSeries`, last 30 seconds of data
4. **Controls row** — Power slider + Intensity slider (disabled when not in Active state)
5. **Action buttons** — START / STOP / EMERGENCY STOP; Emergency Stop triggers a confirmation `Dialog`

**State-driven UI behavior:**
- Sliders disabled in all states except `ACTIVE`
- START button visible only in `READY`
- STOP button visible only in `ACTIVE`
- FAULT overlay (amber banner) shown in `FAULT` state with Acknowledge button
- EMERGENCY STOP lockout overlay (red, full-screen) shown in `EMERGENCY_STOP` state

---

## 5. CI/CD Pipeline

**`ci.yml`** — on every PR and push to `main`:
- Matrix: `ubuntu-latest` + `macos-latest`
- Install Qt 6 via `jurplel/install-qt-action`
- `cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON`
- `cmake --build`
- `ctest --output-on-failure`
- `clang-tidy` on `src/`

**`release.yml`** — on tags matching `v*.*.*`:
- Build on `ubuntu-latest` inside project Dockerfile
- Push to GitHub Container Registry: `ghcr.io/<user>/smart-dental-handpiece:<tag>` + `latest`
- Create GitHub Release with image digest

**Versioning:**
- SemVer tags (`v1.0.0`) drive releases
- `git describe --tags` at CMake configure time sets `PROJECT_VERSION`
- Exposed to app as compile-time `#define APP_VERSION`, displayed in QML status bar

---

## 6. Testing

**`tests/tst_HardwareHandler.cpp`**
- Sensor values stay within valid ranges
- Fault signals fire when thresholds are crossed (injected overheat tick)
- Polling loop emits at expected cadence (QSignalSpy + QTest::qWait)

**`tests/tst_StateManager.cpp`**
- Every valid transition succeeds
- Every invalid transition is silently ignored
- Guard conditions: Emergency Stop fires on temp > 75°C; locks until `clearEmergency()`
- 3 consecutive faults in one session triggers `EMERGENCY_STOP`
- Constructed with `nullptr` for `DatabaseManager` — `StateManager` must null-check before every DB call (a `if (m_db)` guard suffices)

**Runner:** `ctest` wraps Qt Test binaries. CI fails on any non-zero exit.
No QML unit tests — logic lives entirely in C++. README includes a manual smoke test checklist for QML behavior.

---

## 7. File Layout

```
smart_dental_c/
├── CMakeLists.txt
├── README.md
├── Dockerfile
├── .github/
│   └── workflows/
│       ├── ci.yml
│       └── release.yml
├── src/
│   ├── main.cpp
│   ├── HardwareHandler.h
│   ├── HardwareHandler.cpp
│   ├── StateManager.h
│   ├── StateManager.cpp
│   ├── TelemetryBridge.h
│   ├── TelemetryBridge.cpp
│   ├── DatabaseManager.h
│   └── DatabaseManager.cpp
├── qml/
│   ├── main.qml
│   ├── Dashboard.qml
│   └── qml.qrc
├── tests/
│   ├── CMakeLists.txt
│   ├── tst_HardwareHandler.cpp
│   └── tst_StateManager.cpp
├── docs/
│   └── superpowers/
│       └── specs/
│           └── 2026-05-11-smart-dental-design.md
└── data/
    └── .gitkeep
```
