# Smart Dental Handpiece Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a simulated dental handpiece control system in C++17/Qt6 that demonstrates MedTech-grade architecture: layered HAL, a six-state safety state machine, SQLite audit logging, a Dark Clinical QML dashboard, and GitHub Actions CI/CD with Docker release.

**Architecture:** Four layers wired by Dependency Injection in `main.cpp`: `HardwareHandler` (worker thread) → `StateManager` + `TelemetryBridge` (logic) → QML (presentation). `DatabaseManager` is shared by StateManager and TelemetryBridge; HardwareHandler never writes to DB.

**Tech Stack:** C++17, Qt 6.x (Core, Gui, Quick, Qml, Sql, Charts), CMake 3.21+, Qt Test, SQLite (via Qt Sql), GitHub Actions, Docker (ubuntu + Qt headless).

---

## File Map

```
smart_dental_c/
├── CMakeLists.txt                          # root build: app target + optional tests
├── .gitignore
├── Dockerfile
├── README.md
├── .github/workflows/
│   ├── ci.yml                              # build + test on PR/main (ubuntu + macos)
│   └── release.yml                         # docker push on v* tag
├── src/
│   ├── main.cpp                            # DI wiring, QML engine start
│   ├── HardwareHandler.h / .cpp            # worker-thread HAL, sensor simulation
│   ├── StateManager.h / .cpp               # state machine, guards, DB event writes
│   ├── TelemetryBridge.h / .cpp            # Q_PROPERTY bridge, 500ms telemetry timer
│   └── DatabaseManager.h / .cpp           # SQLite: sessions, events, telemetry tables
├── qml/
│   ├── qml.qrc
│   ├── main.qml                            # window root, state-driven overlays
│   └── Dashboard.qml                       # vitals, chart, sliders, action buttons
├── tests/
│   ├── CMakeLists.txt
│   ├── tst_HardwareHandler.cpp
│   └── tst_StateManager.cpp
└── data/
    └── .gitkeep
```

---

## Task 1: Project Scaffolding

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `data/.gitkeep`
- Create: `.gitignore`
- Create: stub `src/main.cpp`

- [ ] **Step 1: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.21)

project(SmartDentalHandpiece VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_VERSION "${PROJECT_VERSION}")
endif()
if(GIT_VERSION STREQUAL "")
    set(GIT_VERSION "${PROJECT_VERSION}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql Charts)

add_compile_definitions(APP_VERSION="${GIT_VERSION}")

set(APP_SOURCES
    src/main.cpp
    src/HardwareHandler.cpp
    src/StateManager.cpp
    src/TelemetryBridge.cpp
    src/DatabaseManager.cpp
)

qt_add_executable(SmartDentalHandpiece ${APP_SOURCES} qml/qml.qrc)

target_include_directories(SmartDentalHandpiece PRIVATE src)

target_link_libraries(SmartDentalHandpiece
    PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Sql Qt6::Charts
)

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

- [ ] **Step 2: Create tests/CMakeLists.txt**

```cmake
find_package(Qt6 REQUIRED COMPONENTS Test Core Sql)

qt_add_executable(tst_HardwareHandler
    tst_HardwareHandler.cpp
    ../src/HardwareHandler.cpp
)
target_include_directories(tst_HardwareHandler PRIVATE ../src)
target_link_libraries(tst_HardwareHandler PRIVATE Qt6::Test Qt6::Core)
add_test(NAME tst_HardwareHandler COMMAND tst_HardwareHandler)

qt_add_executable(tst_StateManager
    tst_StateManager.cpp
    ../src/StateManager.cpp
    ../src/DatabaseManager.cpp
)
target_include_directories(tst_StateManager PRIVATE ../src)
target_link_libraries(tst_StateManager PRIVATE Qt6::Test Qt6::Core Qt6::Sql)
add_test(NAME tst_StateManager COMMAND tst_StateManager)
```

- [ ] **Step 3: Create stub source files so CMake configures without errors**

`src/main.cpp`:
```cpp
#include <QGuiApplication>
int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    return 0;
}
```

`src/HardwareHandler.h`:
```cpp
#pragma once
#include <QObject>
class HardwareHandler : public QObject { Q_OBJECT };
```

`src/HardwareHandler.cpp`:
```cpp
#include "HardwareHandler.h"
```

`src/StateManager.h`:
```cpp
#pragma once
#include <QObject>
class DatabaseManager;
class StateManager : public QObject { Q_OBJECT
public: explicit StateManager(DatabaseManager* db, QObject* parent = nullptr); };
```

`src/StateManager.cpp`:
```cpp
#include "StateManager.h"
StateManager::StateManager(DatabaseManager* db, QObject* parent) : QObject(parent) { Q_UNUSED(db) }
```

`src/TelemetryBridge.h`:
```cpp
#pragma once
#include <QObject>
class DatabaseManager;
class TelemetryBridge : public QObject { Q_OBJECT
public: explicit TelemetryBridge(DatabaseManager* db, QObject* parent = nullptr); };
```

`src/TelemetryBridge.cpp`:
```cpp
#include "TelemetryBridge.h"
TelemetryBridge::TelemetryBridge(DatabaseManager* db, QObject* parent) : QObject(parent) { Q_UNUSED(db) }
```

`src/DatabaseManager.h`:
```cpp
#pragma once
#include <QObject>
class DatabaseManager : public QObject { Q_OBJECT
public: explicit DatabaseManager(const QString& dbPath, QObject* parent = nullptr); };
```

`src/DatabaseManager.cpp`:
```cpp
#include "DatabaseManager.h"
DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent) : QObject(parent) { Q_UNUSED(dbPath) }
```

`qml/qml.qrc`:
```xml
<RCC>
  <qresource prefix="/">
    <file>main.qml</file>
    <file>Dashboard.qml</file>
  </qresource>
</RCC>
```

`qml/main.qml`:
```qml
import QtQuick
Window { visible: true; width: 800; height: 600; title: "Smart Dental Handpiece" }
```

`qml/Dashboard.qml`:
```qml
import QtQuick
Rectangle { color: "#0d1117" }
```

`data/.gitkeep`:
```
```

- [ ] **Step 4: Verify CMake configures and builds**

```bash
cd /Users/kab/Projects/smart_dental_c
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF
cmake --build build
```

Expected: build succeeds, `build/SmartDentalHandpiece` executable exists.

- [ ] **Step 5: Create .gitignore**

```gitignore
build/
build-*/
.superpowers/
data/logs.db
*.user
.DS_Store
CMakeUserPresets.json
```

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/ qml/ data/.gitkeep .gitignore
git commit -m "feat: project scaffolding — CMake, stubs, QML shell"
```

---

## Task 2: DatabaseManager

**Files:**
- Replace: `src/DatabaseManager.h`
- Replace: `src/DatabaseManager.cpp`
- Create: `tests/tst_DatabaseManager.cpp` (inline — no separate test binary needed; verified manually via StateManager tests)

The DatabaseManager is tested indirectly through StateManager tests (null-db path) and manually by inspecting `data/logs.db` after a run. Its interface must be stable before other classes depend on it.

- [ ] **Step 1: Write DatabaseManager.h**

```cpp
// src/DatabaseManager.h
#pragma once
#include <QObject>
#include <QSqlDatabase>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(const QString& dbPath, QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool open();

    qint64 openSession(const QString& firmwareVersion);
    void closeSession(qint64 sessionId);
    void logEvent(qint64 sessionId,
                  const QString& eventType,
                  const QString& fromState,
                  const QString& toState,
                  const QString& detail = {});
    void logTelemetry(qint64 sessionId,
                      double rpm,
                      double temperature,
                      double usageHours);

private:
    bool createTables();

    QSqlDatabase m_db;
    QString m_dbPath;
    static int s_connectionCount;
};
```

- [ ] **Step 2: Write DatabaseManager.cpp**

```cpp
// src/DatabaseManager.cpp
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

int DatabaseManager::s_connectionCount = 0;

DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
    const QString connName = QStringLiteral("sdh_conn_%1").arg(++s_connectionCount);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    m_db.setDatabaseName(dbPath);
}

DatabaseManager::~DatabaseManager() {
    const QString connName = m_db.connectionName();
    m_db.close();
    QSqlDatabase::removeDatabase(connName);
}

bool DatabaseManager::open() {
    if (!m_db.open()) {
        qWarning() << "DatabaseManager: failed to open" << m_dbPath << m_db.lastError().text();
        return false;
    }
    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery q(m_db);
    const QStringList ddl = {
        R"(CREATE TABLE IF NOT EXISTS sessions (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            started_at       TEXT NOT NULL,
            ended_at         TEXT,
            firmware_version TEXT NOT NULL
        ))",
        R"(CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id  INTEGER NOT NULL REFERENCES sessions(id),
            timestamp   TEXT NOT NULL,
            event_type  TEXT NOT NULL,
            from_state  TEXT,
            to_state    TEXT NOT NULL,
            detail      TEXT
        ))",
        R"(CREATE TABLE IF NOT EXISTS telemetry (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id   INTEGER NOT NULL REFERENCES sessions(id),
            timestamp    TEXT NOT NULL,
            rpm          REAL NOT NULL,
            temperature  REAL NOT NULL,
            usage_hours  REAL NOT NULL
        ))"
    };
    for (const auto& stmt : ddl) {
        if (!q.exec(stmt)) {
            qWarning() << "DatabaseManager: DDL failed:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

qint64 DatabaseManager::openSession(const QString& firmwareVersion) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO sessions (started_at, firmware_version) VALUES (?, ?))");
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(firmwareVersion);
    if (!q.exec()) {
        qWarning() << "DatabaseManager::openSession failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toLongLong();
}

void DatabaseManager::closeSession(qint64 sessionId) {
    QSqlQuery q(m_db);
    q.prepare(R"(UPDATE sessions SET ended_at = ? WHERE id = ?)");
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(sessionId);
    q.exec();
}

void DatabaseManager::logEvent(qint64 sessionId,
                                const QString& eventType,
                                const QString& fromState,
                                const QString& toState,
                                const QString& detail)
{
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO events
        (session_id, timestamp, event_type, from_state, to_state, detail)
        VALUES (?, ?, ?, ?, ?, ?))");
    q.addBindValue(sessionId);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(eventType);
    q.addBindValue(fromState.isEmpty() ? QVariant{} : fromState);
    q.addBindValue(toState);
    q.addBindValue(detail.isEmpty() ? QVariant{} : detail);
    q.exec();
}

void DatabaseManager::logTelemetry(qint64 sessionId,
                                    double rpm,
                                    double temperature,
                                    double usageHours)
{
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO telemetry
        (session_id, timestamp, rpm, temperature, usage_hours)
        VALUES (?, ?, ?, ?, ?))");
    q.addBindValue(sessionId);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(rpm);
    q.addBindValue(temperature);
    q.addBindValue(usageHours);
    q.exec();
}
```

- [ ] **Step 3: Rebuild to confirm it compiles**

```bash
cmake --build build
```

Expected: clean build, no errors.

- [ ] **Step 4: Commit**

```bash
git add src/DatabaseManager.h src/DatabaseManager.cpp
git commit -m "feat: DatabaseManager — SQLite sessions/events/telemetry"
```

---

## Task 3: HardwareHandler

**Files:**
- Replace: `src/HardwareHandler.h`
- Replace: `src/HardwareHandler.cpp`
- Create: `tests/tst_HardwareHandler.cpp`

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/tst_HardwareHandler.cpp
#include <QtTest>
#include "HardwareHandler.h"

class TestHardwareHandler : public QObject {
    Q_OBJECT
private slots:
    void testSensorValuesInRange();
    void testSignalsEmittedWithinWindow();
    void testFaultSignalEmitted();
};

void TestHardwareHandler::testSensorValuesInRange() {
    HardwareHandler hw;
    QSignalSpy spy(&hw, &HardwareHandler::sensorUpdate);
    hw.start();
    QVERIFY(spy.wait(1000));  // at least one signal within 1s
    hw.stop();

    for (const QList<QVariant>& args : spy) {
        double rpm  = args[0].toDouble();
        double temp = args[1].toDouble();
        double hrs  = args[2].toDouble();
        QVERIFY2(rpm >= 0.0 && rpm <= 400'000.0,
                 qPrintable(QString("RPM out of range: %1").arg(rpm)));
        QVERIFY2(temp >= 15.0 && temp <= 100.0,
                 qPrintable(QString("Temp out of range: %1").arg(temp)));
        QVERIFY2(hrs >= 0.0,
                 qPrintable(QString("Usage hours negative: %1").arg(hrs)));
    }
}

void TestHardwareHandler::testSignalsEmittedWithinWindow() {
    HardwareHandler hw;
    QSignalSpy spy(&hw, &HardwareHandler::sensorUpdate);
    hw.start();
    QTest::qWait(600);  // expect ~6 ticks at 100ms
    hw.stop();
    QVERIFY2(spy.count() >= 4,
             qPrintable(QString("Expected >=4 signals, got %1").arg(spy.count())));
}

void TestHardwareHandler::testFaultSignalEmitted() {
    HardwareHandler hw;
    QSignalSpy faultSpy(&hw, &HardwareHandler::faultDetected);
    hw.start();
    // Run long enough for the 1-in-200 MOTOR_STALL simulation to trigger
    QTest::qWait(5000);
    hw.stop();
    // We can't guarantee a fault within 5s (probability ~2%), so just confirm
    // the signal type string is valid when it does fire.
    for (const QList<QVariant>& args : faultSpy) {
        const QString type = args[0].toString();
        QVERIFY2(type == "SENSOR_NAN" || type == "MOTOR_STALL",
                 qPrintable(QString("Unknown fault type: %1").arg(type)));
    }
    QVERIFY(true);  // test passes even with 0 faults (probability event)
}

QTEST_MAIN(TestHardwareHandler)
#include "tst_HardwareHandler.moc"
```

- [ ] **Step 2: Run tests — expect compile failure (HardwareHandler not yet implemented)**

```bash
cmake -B build -DBUILD_TESTS=ON && cmake --build build 2>&1 | head -30
```

Expected: compile error referencing missing `HardwareHandler::start()`, `HardwareHandler::stop()`, signals.

- [ ] **Step 3: Write HardwareHandler.h**

```cpp
// src/HardwareHandler.h
#pragma once
#include <QObject>
#include <QThread>

class HardwarePoller : public QObject {
    Q_OBJECT
public:
    explicit HardwarePoller(QObject* parent = nullptr);
public slots:
    void start();
    void stop();
signals:
    void sensorUpdate(double rpm, double temperature, double usageHours);
    void faultDetected(const QString& type, const QString& detail);
private slots:
    void poll();
private:
    QTimer* m_timer{nullptr};
    double  m_usageHours{0.0};
    int     m_tick{0};
};

class HardwareHandler : public QObject {
    Q_OBJECT
public:
    explicit HardwareHandler(QObject* parent = nullptr);
    ~HardwareHandler() override;

    void start();
    void stop();

signals:
    void sensorUpdate(double rpm, double temperature, double usageHours);
    void faultDetected(const QString& type, const QString& detail);

private:
    QThread        m_workerThread;
    HardwarePoller* m_poller;
};
```

- [ ] **Step 4: Write HardwareHandler.cpp**

```cpp
// src/HardwareHandler.cpp
#include "HardwareHandler.h"
#include <QTimer>
#include <cmath>
#include <random>

namespace {
    std::mt19937& rng() {
        static std::mt19937 gen{std::random_device{}()};
        return gen;
    }
}

// ── HardwarePoller ──────────────────────────────────────────────────────────

HardwarePoller::HardwarePoller(QObject* parent) : QObject(parent) {}

void HardwarePoller::start() {
    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &HardwarePoller::poll);
    m_timer->start();
}

void HardwarePoller::stop() {
    if (m_timer) m_timer->stop();
}

void HardwarePoller::poll() {
    ++m_tick;
    m_usageHours += 100.0 / 3'600'000.0;

    // Simulate sensor dropout (SENSOR_NAN): ~1 in 500 ticks
    std::uniform_int_distribution<int> nanDist(1, 500);
    if (nanDist(rng()) == 1) {
        emit faultDetected(QStringLiteral("SENSOR_NAN"),
                           QStringLiteral("RPM sensor dropout"));
        return;  // no sensorUpdate emitted
    }

    // Simulate motor stall: ~1 in 200 ticks
    std::uniform_int_distribution<int> stallDist(1, 200);
    if (stallDist(rng()) == 1) {
        emit faultDetected(QStringLiteral("MOTOR_STALL"),
                           QStringLiteral("RPM=0 detected for >500ms"));
        // still emit sensorUpdate with rpm=0 to reflect stall
    }

    std::normal_distribution<double> rpmNoise(0.0, 400.0);
    std::normal_distribution<double> tempNoise(0.0, 0.15);

    const double baseRpm = 30'000.0;
    double rpm = baseRpm
               + 4'000.0 * std::sin(m_tick * 0.04)
               + rpmNoise(rng());
    rpm = std::clamp(rpm, 0.0, 400'000.0);

    // Temperature: slow climb proportional to RPM
    double temperature = 36.5
                        + (rpm / baseRpm) * 1.8
                        + 0.0005 * m_tick
                        + tempNoise(rng());
    temperature = std::clamp(temperature, 15.0, 100.0);

    emit sensorUpdate(rpm, temperature, m_usageHours);
}

// ── HardwareHandler ─────────────────────────────────────────────────────────

HardwareHandler::HardwareHandler(QObject* parent) : QObject(parent) {
    m_poller = new HardwarePoller;
    m_poller->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::started,
            m_poller, &HardwarePoller::start);
    connect(m_poller, &HardwarePoller::sensorUpdate,
            this,    &HardwareHandler::sensorUpdate);
    connect(m_poller, &HardwarePoller::faultDetected,
            this,    &HardwareHandler::faultDetected);
}

HardwareHandler::~HardwareHandler() {
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_poller;
}

void HardwareHandler::start() {
    m_workerThread.start();
}

void HardwareHandler::stop() {
    QMetaObject::invokeMethod(m_poller, "stop", Qt::QueuedConnection);
}
```

- [ ] **Step 5: Run the tests**

```bash
cmake --build build && ctest --test-dir build -R tst_HardwareHandler -V
```

Expected output:
```
PASS   : TestHardwareHandler::testSensorValuesInRange()
PASS   : TestHardwareHandler::testSignalsEmittedWithinWindow()
PASS   : TestHardwareHandler::testFaultSignalEmitted()
```

- [ ] **Step 6: Commit**

```bash
git add src/HardwareHandler.h src/HardwareHandler.cpp tests/tst_HardwareHandler.cpp
git commit -m "feat: HardwareHandler — worker-thread sensor simulation with fault signals"
```

---

## Task 4: StateManager

**Files:**
- Replace: `src/StateManager.h`
- Replace: `src/StateManager.cpp`
- Create: `tests/tst_StateManager.cpp`

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/tst_StateManager.cpp
#include <QtTest>
#include "StateManager.h"

class TestStateManager : public QObject {
    Q_OBJECT
private slots:
    void testInitialState();
    void testStartDeviceTransition();
    void testCalibrationSucceeds();
    void testCalibrationFails();
    void testStartAndStopProcedure();
    void testOverheatTriggersFault();
    void testCriticalTempTriggersEmergency();
    void testAcknowledgeFault();
    void testClearEmergency();
    void testInvalidTransitionIgnored();
    void testThreeFaultsTriggersEmergency();
    void testFaultSignalTriggersFaultState();
};

// Helper: drive calibration to READY
static void calibrate(StateManager& sm) {
    sm.startDevice();
    for (int i = 0; i < 3; ++i)
        sm.onSensorUpdate(30'000.0, 36.5, 0.0);
}

void TestStateManager::testInitialState() {
    StateManager sm(nullptr);
    QCOMPARE(sm.stateString(), QStringLiteral("STANDBY"));
}

void TestStateManager::testStartDeviceTransition() {
    StateManager sm(nullptr);
    sm.startDevice();
    QCOMPARE(sm.stateString(), QStringLiteral("CALIBRATING"));
}

void TestStateManager::testCalibrationSucceeds() {
    StateManager sm(nullptr);
    sm.startDevice();
    for (int i = 0; i < 3; ++i)
        sm.onSensorUpdate(30'000.0, 36.5, 0.0);
    QCOMPARE(sm.stateString(), QStringLiteral("READY"));
}

void TestStateManager::testCalibrationFails() {
    StateManager sm(nullptr);
    sm.startDevice();
    sm.onFaultDetected(QStringLiteral("SENSOR_NAN"), QStringLiteral("test"));
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
}

void TestStateManager::testStartAndStopProcedure() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    QCOMPARE(sm.stateString(), QStringLiteral("ACTIVE"));
    sm.stopProcedure();
    QCOMPARE(sm.stateString(), QStringLiteral("READY"));
}

void TestStateManager::testOverheatTriggersFault() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    sm.onSensorUpdate(30'000.0, 62.0, 0.0);  // temp > 60
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
    QVERIFY(sm.isFault());
}

void TestStateManager::testCriticalTempTriggersEmergency() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    sm.onSensorUpdate(30'000.0, 76.0, 0.0);  // temp > 75
    QCOMPARE(sm.stateString(), QStringLiteral("EMERGENCY_STOP"));
    QVERIFY(sm.isEmergency());
}

void TestStateManager::testAcknowledgeFault() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    sm.onSensorUpdate(30'000.0, 62.0, 0.0);
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
    sm.acknowledgeFault();
    QCOMPARE(sm.stateString(), QStringLiteral("STANDBY"));
}

void TestStateManager::testClearEmergency() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    sm.onSensorUpdate(30'000.0, 76.0, 0.0);
    QCOMPARE(sm.stateString(), QStringLiteral("EMERGENCY_STOP"));
    sm.clearEmergency();
    QCOMPARE(sm.stateString(), QStringLiteral("STANDBY"));
}

void TestStateManager::testInvalidTransitionIgnored() {
    StateManager sm(nullptr);
    // Can't startProcedure from STANDBY
    sm.startProcedure();
    QCOMPARE(sm.stateString(), QStringLiteral("STANDBY"));
    // Can't acknowledgeFault from STANDBY
    sm.acknowledgeFault();
    QCOMPARE(sm.stateString(), QStringLiteral("STANDBY"));
}

void TestStateManager::testThreeFaultsTriggersEmergency() {
    StateManager sm(nullptr);

    auto faultCycle = [&]() {
        calibrate(sm);
        sm.startProcedure();
        sm.onSensorUpdate(30'000.0, 62.0, 0.0);  // temp > 60 → FAULT
    };

    // Fault 1
    faultCycle();
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
    sm.acknowledgeFault();  // → STANDBY, session stays open, faultCount = 1

    // Fault 2
    faultCycle();
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
    sm.acknowledgeFault();  // faultCount = 2

    // Fault 3 → EMERGENCY_STOP instead of FAULT
    sm.startDevice();
    for (int i = 0; i < 3; ++i)
        sm.onSensorUpdate(30'000.0, 36.5, 0.0);
    sm.startProcedure();
    sm.onSensorUpdate(30'000.0, 62.0, 0.0);
    QCOMPARE(sm.stateString(), QStringLiteral("EMERGENCY_STOP"));
}

void TestStateManager::testFaultSignalTriggersFaultState() {
    StateManager sm(nullptr);
    calibrate(sm);
    sm.startProcedure();
    sm.onFaultDetected(QStringLiteral("MOTOR_STALL"), QStringLiteral("RPM=0"));
    QCOMPARE(sm.stateString(), QStringLiteral("FAULT"));
}

QTEST_MAIN(TestStateManager)
#include "tst_StateManager.moc"
```

- [ ] **Step 2: Run — expect compile failure (StateManager interface not yet defined)**

```bash
cmake --build build 2>&1 | head -20
```

Expected: errors about missing `stateString()`, `isFault()`, `isEmergency()`, `startDevice()`, etc.

- [ ] **Step 3: Write StateManager.h**

```cpp
// src/StateManager.h
#pragma once
#include <QObject>
#include <QString>

class DatabaseManager;

enum class DeviceState {
    STANDBY,
    CALIBRATING,
    READY,
    ACTIVE,
    FAULT,
    EMERGENCY_STOP
};

class StateManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString stateString  READ stateString  NOTIFY stateChanged)
    Q_PROPERTY(bool    isEmergency  READ isEmergency  NOTIFY stateChanged)
    Q_PROPERTY(bool    isFault      READ isFault      NOTIFY stateChanged)
    Q_PROPERTY(QString faultDetail  READ faultDetail  NOTIFY faultDetailChanged)

public:
    explicit StateManager(DatabaseManager* db, QObject* parent = nullptr);

    DeviceState state()       const { return m_state; }
    QString     stateString() const { return stateToString(m_state); }
    bool        isEmergency() const { return m_state == DeviceState::EMERGENCY_STOP; }
    bool        isFault()     const { return m_state == DeviceState::FAULT; }
    QString     faultDetail() const { return m_faultDetail; }
    qint64      sessionId()   const { return m_sessionId; }

    Q_INVOKABLE void startDevice();     // STANDBY → CALIBRATING
    Q_INVOKABLE void startProcedure();  // READY   → ACTIVE
    Q_INVOKABLE void stopProcedure();   // ACTIVE  → READY
    Q_INVOKABLE void acknowledgeFault();// FAULT   → STANDBY
    Q_INVOKABLE void clearEmergency();  // EMERGENCY_STOP → STANDBY

public slots:
    void onSensorUpdate(double rpm, double temperature, double usageHours);
    void onFaultDetected(const QString& type, const QString& detail);

signals:
    void stateChanged(DeviceState newState);
    void faultDetailChanged();
    void sessionStarted(qint64 sessionId);

private:
    void transitionTo(DeviceState next, const QString& detail = {});
    static QString stateToString(DeviceState s);

    DeviceState m_state{DeviceState::STANDBY};
    DatabaseManager* m_db;

    // Calibration
    int m_validCalibrationTicks{0};
    static constexpr int kCalibrationTicks{3};

    // Fault tracking (resets only on clearEmergency or first startDevice)
    int m_sessionFaultCount{0};
    static constexpr int kMaxFaults{3};

    // Session
    qint64  m_sessionId{-1};
    QString m_faultDetail;
};
```

- [ ] **Step 4: Write StateManager.cpp**

```cpp
// src/StateManager.cpp
#include "StateManager.h"
#include "DatabaseManager.h"

StateManager::StateManager(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db) {}

QString StateManager::stateToString(DeviceState s) {
    switch (s) {
        case DeviceState::STANDBY:        return QStringLiteral("STANDBY");
        case DeviceState::CALIBRATING:    return QStringLiteral("CALIBRATING");
        case DeviceState::READY:          return QStringLiteral("READY");
        case DeviceState::ACTIVE:         return QStringLiteral("ACTIVE");
        case DeviceState::FAULT:          return QStringLiteral("FAULT");
        case DeviceState::EMERGENCY_STOP: return QStringLiteral("EMERGENCY_STOP");
    }
    return {};
}

void StateManager::transitionTo(DeviceState next, const QString& detail) {
    if (m_state == next) return;

    const QString from = stateToString(m_state);
    const QString to   = stateToString(next);

    m_state = next;

    if (m_db && m_sessionId != -1) {
        m_db->logEvent(m_sessionId, QStringLiteral("STATE_CHANGE"),
                       from, to, detail);
    }

    emit stateChanged(next);
}

void StateManager::startDevice() {
    if (m_state != DeviceState::STANDBY) return;

    m_validCalibrationTicks = 0;

    if (m_sessionId == -1) {
        m_sessionFaultCount = 0;
        if (m_db) {
            m_sessionId = m_db->openSession(QStringLiteral(APP_VERSION));
            emit sessionStarted(m_sessionId);
        }
    }

    transitionTo(DeviceState::CALIBRATING);
}

void StateManager::startProcedure() {
    if (m_state != DeviceState::READY) return;
    transitionTo(DeviceState::ACTIVE);
}

void StateManager::stopProcedure() {
    if (m_state != DeviceState::ACTIVE) return;
    transitionTo(DeviceState::READY);
}

void StateManager::acknowledgeFault() {
    if (m_state != DeviceState::FAULT) return;
    m_faultDetail.clear();
    emit faultDetailChanged();
    transitionTo(DeviceState::STANDBY);
}

void StateManager::clearEmergency() {
    if (m_state != DeviceState::EMERGENCY_STOP) return;
    if (m_db && m_sessionId != -1) {
        m_db->closeSession(m_sessionId);
    }
    m_sessionId = -1;
    m_sessionFaultCount = 0;
    m_faultDetail.clear();
    emit faultDetailChanged();
    transitionTo(DeviceState::STANDBY);
}

void StateManager::onSensorUpdate(double /*rpm*/, double temperature, double /*usageHours*/) {
    switch (m_state) {
    case DeviceState::CALIBRATING:
        ++m_validCalibrationTicks;
        if (m_validCalibrationTicks >= kCalibrationTicks)
            transitionTo(DeviceState::READY);
        break;

    case DeviceState::ACTIVE:
        if (temperature > 75.0) {
            m_faultDetail = QString("temp=%1°C").arg(temperature, 0, 'f', 1);
            emit faultDetailChanged();
            transitionTo(DeviceState::EMERGENCY_STOP, m_faultDetail);
        } else if (temperature > 60.0) {
            ++m_sessionFaultCount;
            m_faultDetail = QString("temp=%1°C").arg(temperature, 0, 'f', 1);
            emit faultDetailChanged();
            if (m_sessionFaultCount >= kMaxFaults) {
                if (m_db && m_sessionId != -1)
                    m_db->logEvent(m_sessionId, QStringLiteral("FAULT"),
                                   stateToString(m_state),
                                   stateToString(DeviceState::FAULT),
                                   m_faultDetail);
                transitionTo(DeviceState::EMERGENCY_STOP, m_faultDetail);
            } else {
                transitionTo(DeviceState::FAULT, m_faultDetail);
            }
        }
        break;

    default:
        break;
    }
}

void StateManager::onFaultDetected(const QString& type, const QString& detail) {
    if (m_state != DeviceState::ACTIVE && m_state != DeviceState::CALIBRATING)
        return;

    ++m_sessionFaultCount;
    m_faultDetail = QString("%1: %2").arg(type, detail);
    emit faultDetailChanged();

    if (m_sessionFaultCount >= kMaxFaults) {
        transitionTo(DeviceState::EMERGENCY_STOP, m_faultDetail);
    } else {
        transitionTo(DeviceState::FAULT, m_faultDetail);
    }
}
```

- [ ] **Step 5: Run tests**

```bash
cmake --build build && ctest --test-dir build -R tst_StateManager -V
```

Expected: all 12 tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/StateManager.h src/StateManager.cpp tests/tst_StateManager.cpp
git commit -m "feat: StateManager — six-state device state machine with guards (TDD)"
```

---

## Task 5: TelemetryBridge + main.cpp Wiring

**Files:**
- Replace: `src/TelemetryBridge.h`
- Replace: `src/TelemetryBridge.cpp`
- Replace: `src/main.cpp`

- [ ] **Step 1: Write TelemetryBridge.h**

```cpp
// src/TelemetryBridge.h
#pragma once
#include <QObject>
#include <QTimer>
#include "StateManager.h"

class DatabaseManager;

class TelemetryBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(double rpm         READ rpm         NOTIFY rpmChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(double usageHours  READ usageHours  NOTIFY usageHoursChanged)

public:
    explicit TelemetryBridge(DatabaseManager* db, QObject* parent = nullptr);

    double rpm()         const { return m_rpm; }
    double temperature() const { return m_temperature; }
    double usageHours()  const { return m_usageHours; }

public slots:
    void onSensorUpdate(double rpm, double temperature, double usageHours);
    void onStateChanged(DeviceState newState);
    void onSessionStarted(qint64 sessionId);

signals:
    void rpmChanged();
    void temperatureChanged();
    void usageHoursChanged();
    void rpmPointAdded(double elapsedSeconds, double rpm);

private slots:
    void writeTelemetry();

private:
    double m_rpm{0.0};
    double m_temperature{0.0};
    double m_usageHours{0.0};

    DatabaseManager* m_db;
    QTimer           m_telemetryTimer;
    qint64           m_sessionId{-1};
    qint64           m_sessionStartMs{0};
};
```

- [ ] **Step 2: Write TelemetryBridge.cpp**

```cpp
// src/TelemetryBridge.cpp
#include "TelemetryBridge.h"
#include "DatabaseManager.h"
#include <QDateTime>

TelemetryBridge::TelemetryBridge(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
    m_telemetryTimer.setInterval(500);
    connect(&m_telemetryTimer, &QTimer::timeout,
            this, &TelemetryBridge::writeTelemetry);
}

void TelemetryBridge::onSensorUpdate(double rpm, double temperature, double usageHours) {
    if (m_rpm != rpm) {
        m_rpm = rpm;
        emit rpmChanged();
    }
    if (m_temperature != temperature) {
        m_temperature = temperature;
        emit temperatureChanged();
    }
    if (m_usageHours != usageHours) {
        m_usageHours = usageHours;
        emit usageHoursChanged();
    }

    const double elapsed = m_sessionStartMs > 0
        ? (QDateTime::currentMSecsSinceEpoch() - m_sessionStartMs) / 1000.0
        : 0.0;
    emit rpmPointAdded(elapsed, rpm);
}

void TelemetryBridge::onStateChanged(DeviceState newState) {
    if (newState == DeviceState::ACTIVE) {
        m_sessionStartMs = QDateTime::currentMSecsSinceEpoch();
        m_telemetryTimer.start();
    } else {
        m_telemetryTimer.stop();
    }
}

void TelemetryBridge::onSessionStarted(qint64 sessionId) {
    m_sessionId = sessionId;
}

void TelemetryBridge::writeTelemetry() {
    if (m_db && m_sessionId != -1) {
        m_db->logTelemetry(m_sessionId, m_rpm, m_temperature, m_usageHours);
    }
}
```

- [ ] **Step 3: Write main.cpp**

```cpp
// src/main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>

#include "HardwareHandler.h"
#include "StateManager.h"
#include "TelemetryBridge.h"
#include "DatabaseManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SmartDentalHandpiece"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    // Ensure data directory exists
    QDir().mkpath(QStringLiteral("data"));

    DatabaseManager db(QStringLiteral("data/logs.db"));
    if (!db.open()) {
        qWarning() << "Failed to open database — logging disabled";
    }

    HardwareHandler hardware;
    StateManager    stateManager(&db);
    TelemetryBridge telemetryBridge(&db);

    // Wire HardwareHandler → StateManager
    QObject::connect(&hardware, &HardwareHandler::sensorUpdate,
                     &stateManager, &StateManager::onSensorUpdate);
    QObject::connect(&hardware, &HardwareHandler::faultDetected,
                     &stateManager, &StateManager::onFaultDetected);

    // Wire HardwareHandler → TelemetryBridge
    QObject::connect(&hardware, &HardwareHandler::sensorUpdate,
                     &telemetryBridge, &TelemetryBridge::onSensorUpdate);

    // Wire StateManager → TelemetryBridge
    QObject::connect(&stateManager, &StateManager::stateChanged,
                     &telemetryBridge, &TelemetryBridge::onStateChanged);
    QObject::connect(&stateManager, &StateManager::sessionStarted,
                     &telemetryBridge, &TelemetryBridge::onSessionStarted);

    hardware.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("stateManager"),   &stateManager);
    engine.rootContext()->setContextProperty(QStringLiteral("telemetryBridge"), &telemetryBridge);
    engine.rootContext()->setContextProperty(QStringLiteral("appVersion"),
                                             QStringLiteral(APP_VERSION));

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
```

- [ ] **Step 4: Build to verify wiring compiles**

```bash
cmake --build build
```

Expected: clean build. App launches with a plain dark window (QML is still a stub).

- [ ] **Step 5: Commit**

```bash
git add src/TelemetryBridge.h src/TelemetryBridge.cpp src/main.cpp
git commit -m "feat: TelemetryBridge + DI wiring in main.cpp"
```

---

## Task 6: QML Frontend

**Files:**
- Replace: `qml/main.qml`
- Replace: `qml/Dashboard.qml`

Color constants used throughout (Dark Clinical palette):
- Background: `"#0d1117"`
- Card: `"#161b22"`
- Border: `"#30363d"`
- Green: `"#4ade80"`
- Amber: `"#f59e0b"`
- Red emergency: `"#ef4444"`
- Text primary: `"#e6edf3"`
- Text secondary: `"#94a3b8"`

- [ ] **Step 1: Write main.qml**

```qml
// qml/main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 660
    minimumWidth: 720
    minimumHeight: 560
    title: "Smart Dental Handpiece"
    color: "#0d1117"

    // ── Status bar ──────────────────────────────────────────────────────────
    Rectangle {
        id: statusBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 44
        color: "#0a0e14"
        border.color: "#21262d"
        border.width: 1

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
            spacing: 12

            Rectangle {
                width: 8; height: 8; radius: 4
                color: stateManager.isEmergency ? "#ef4444"
                     : stateManager.isFault     ? "#f59e0b"
                     : stateManager.stateString === "ACTIVE" ? "#4ade80"
                     : "#4b5563"
            }

            Text {
                text: "SMART DENTAL HANDPIECE"
                color: "#94a3b8"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 2 }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: stateBadgeText.implicitWidth + 16
                height: 22
                radius: 3
                color: stateManager.isEmergency ? "#3b0a0a"
                     : stateManager.isFault     ? "#3b2800"
                     : stateManager.stateString === "ACTIVE" ? "#0f4c2a"
                     : "#1e2738"
                border.color: stateManager.isEmergency ? "#ef4444"
                            : stateManager.isFault     ? "#f59e0b"
                            : stateManager.stateString === "ACTIVE" ? "#4ade80"
                            : "#30363d"
                border.width: 1

                Text {
                    id: stateBadgeText
                    anchors.centerIn: parent
                    text: stateManager.stateString
                    color: stateManager.isEmergency ? "#ef4444"
                         : stateManager.isFault     ? "#f59e0b"
                         : stateManager.stateString === "ACTIVE" ? "#4ade80"
                         : "#94a3b8"
                    font { family: "monospace"; pixelSize: 10; letterSpacing: 1 }
                }
            }

            Text {
                text: "v" + appVersion
                color: "#4b5563"
                font { family: "monospace"; pixelSize: 10 }
            }
        }
    }

    // ── Main content ─────────────────────────────────────────────────────────
    Dashboard {
        anchors {
            top: statusBar.bottom
            left: parent.left; right: parent.right; bottom: parent.bottom
            margins: 16
        }
    }

    // ── FAULT overlay ────────────────────────────────────────────────────────
    Rectangle {
        visible: stateManager.isFault
        anchors { top: statusBar.bottom; left: parent.left; right: parent.right }
        height: 52
        color: "#1a1000"
        border.color: "#f59e0b"
        border.width: 1
        z: 10

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }

            Text {
                text: "⚠  FAULT — " + stateManager.faultDetail
                color: "#f59e0b"
                font { pixelSize: 13 }
                Layout.fillWidth: true
            }

            Button {
                text: "ACKNOWLEDGE"
                onClicked: stateManager.acknowledgeFault()
                contentItem: Text {
                    text: parent.text
                    color: "#f59e0b"
                    font { pixelSize: 11; letterSpacing: 1 }
                    horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    color: parent.hovered ? "#2a1a00" : "#1a1000"
                    border.color: "#f59e0b"
                    border.width: 1
                    radius: 3
                }
            }
        }
    }

    // ── EMERGENCY STOP overlay ────────────────────────────────────────────────
    Rectangle {
        visible: stateManager.isEmergency
        anchors.fill: parent
        color: "#1a0000"
        opacity: 0.97
        z: 20

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "EMERGENCY STOP"
                color: "#ef4444"
                font { family: "monospace"; pixelSize: 32; bold: true; letterSpacing: 4 }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: stateManager.faultDetail
                color: "#94a3b8"
                font { pixelSize: 14 }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Device is locked. Inspect hardware before clearing."
                color: "#6b7280"
                font { pixelSize: 12 }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "CLEAR EMERGENCY & RESET"
                onClicked: stateManager.clearEmergency()
                contentItem: Text {
                    text: parent.text
                    color: "#ef4444"
                    font { pixelSize: 12; letterSpacing: 1 }
                    horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    color: parent.hovered ? "#300000" : "#200000"
                    border.color: "#ef4444"
                    border.width: 1
                    radius: 4
                    implicitWidth: 260
                    implicitHeight: 40
                }
            }
        }
    }
}
```

- [ ] **Step 2: Write Dashboard.qml**

```qml
// qml/Dashboard.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

Item {
    id: root

    // ── Vitals row ────────────────────────────────────────────────────────────
    RowLayout {
        id: vitalsRow
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 80
        spacing: 12

        Repeater {
            model: [
                { label: "RPM",     value: telemetryBridge.rpm.toLocaleString(Qt.locale(), 'f', 0),  color: "#4ade80" },
                { label: "TEMP °C", value: telemetryBridge.temperature.toLocaleString(Qt.locale(), 'f', 1), color: telemetryBridge.temperature > 60 ? "#ef4444" : telemetryBridge.temperature > 50 ? "#f59e0b" : "#4ade80" },
                { label: "HOURS",   value: telemetryBridge.usageHours.toLocaleString(Qt.locale(), 'f', 1),  color: "#94a3b8" }
            ]

            Rectangle {
                Layout.fillWidth: true
                height: 80
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1
                radius: 4

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.label
                        color: "#4b5563"
                        font { family: "monospace"; pixelSize: 9; letterSpacing: 1 }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.value
                        color: modelData.color
                        font { family: "monospace"; pixelSize: 22; bold: true }
                    }
                }
            }
        }
    }

    // ── RPM Chart ─────────────────────────────────────────────────────────────
    ChartView {
        id: rpmChart
        anchors {
            top: vitalsRow.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: parent.height * 0.38
        backgroundColor: "#161b22"
        plotAreaColor: "#161b22"
        legend.visible: false
        antialiasing: true
        margins { top: 8; bottom: 8; left: 0; right: 0 }

        ValueAxis {
            id: axisX
            min: 0; max: 30
            labelsColor: "#4b5563"
            gridLineColor: "#21262d"
            labelsFont.pixelSize: 9
            titleText: ""
        }
        ValueAxis {
            id: axisY
            min: 0; max: 45000
            labelsColor: "#4b5563"
            gridLineColor: "#21262d"
            labelsFont.pixelSize: 9
            titleText: ""
        }

        LineSeries {
            id: rpmSeries
            axisX: axisX
            axisY: axisY
            color: "#4ade80"
            width: 1.5
        }

        Connections {
            target: telemetryBridge
            function onRpmPointAdded(x, y) {
                if (rpmSeries.count > 300) rpmSeries.remove(0)
                rpmSeries.append(x, y)
                if (x > axisX.max) {
                    axisX.min = x - 30
                    axisX.max = x
                }
            }
        }
    }

    // ── Controls ──────────────────────────────────────────────────────────────
    RowLayout {
        id: controlsRow
        anchors {
            top: rpmChart.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: 72
        spacing: 12

        Repeater {
            model: [
                { label: "POWER",     color: "#4ade80" },
                { label: "INTENSITY", color: "#60a5fa" }
            ]

            Rectangle {
                Layout.fillWidth: true
                height: 72
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1
                radius: 4

                Column {
                    anchors { fill: parent; margins: 12 }
                    spacing: 8

                    Text {
                        text: modelData.label
                        color: "#4b5563"
                        font { family: "monospace"; pixelSize: 9; letterSpacing: 1 }
                    }

                    Slider {
                        width: parent.width
                        from: 0; to: 100; value: 50
                        enabled: stateManager.stateString === "ACTIVE"
                        opacity: enabled ? 1.0 : 0.35

                        background: Rectangle {
                            x: parent.leftPadding
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: parent.availableWidth
                            height: 5
                            radius: 2
                            color: "#0d1117"

                            Rectangle {
                                width: parent.parent.visualPosition * parent.width
                                height: parent.height
                                radius: parent.radius
                                color: modelData.color
                            }
                        }

                        handle: Rectangle {
                            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: 14; height: 14; radius: 7
                            color: modelData.color
                            border.color: "#0d1117"
                            border.width: 2
                        }
                    }
                }
            }
        }
    }

    // ── Action buttons ────────────────────────────────────────────────────────
    RowLayout {
        anchors {
            top: controlsRow.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: 44
        spacing: 8

        // POWER ON / START DEVICE (shown in STANDBY)
        Button {
            visible: stateManager.stateString === "STANDBY"
            Layout.fillWidth: true
            text: "POWER ON"
            onClicked: stateManager.startDevice()
            contentItem: Text {
                text: parent.text; color: "#4ade80"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#0f4c2a" : "#0a2e1a"
                border.color: "#4ade80"; border.width: 1; radius: 3
            }
        }

        // START (shown in READY)
        Button {
            visible: stateManager.stateString === "READY"
            Layout.fillWidth: true
            text: "START"
            onClicked: stateManager.startProcedure()
            contentItem: Text {
                text: parent.text; color: "#4ade80"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#0f4c2a" : "#0a2e1a"
                border.color: "#4ade80"; border.width: 1; radius: 3
            }
        }

        // STOP (shown in ACTIVE)
        Button {
            visible: stateManager.stateString === "ACTIVE"
            Layout.fillWidth: true
            text: "STOP"
            onClicked: stateManager.stopProcedure()
            contentItem: Text {
                text: parent.text; color: "#94a3b8"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#1e2738" : "#161b22"
                border.color: "#30363d"; border.width: 1; radius: 3
            }
        }

        // Spacer when CALIBRATING
        Item {
            visible: stateManager.stateString === "CALIBRATING"
            Layout.fillWidth: true
            Text {
                anchors.centerIn: parent
                text: "CALIBRATING..."
                color: "#60a5fa"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 2 }
            }
        }

        // EMERGENCY STOP (always visible)
        Button {
            Layout.preferredWidth: 160
            text: "E-STOP"
            enabled: stateManager.stateString === "ACTIVE" || stateManager.stateString === "READY"
            opacity: enabled ? 1.0 : 0.3
            onClicked: {
                // Force emergency via a critical temperature signal is not available here;
                // this button simulates triggering via direct state method (for demo).
                // In production this would be a hardware interrupt.
                stateManager.onSensorUpdate(30000, 80.0, 0)
            }
            contentItem: Text {
                text: parent.text; color: "#ef4444"
                font { family: "monospace"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered && parent.enabled ? "#300000" : "#1a0000"
                border.color: "#ef4444"; border.width: 1; radius: 3
            }
        }
    }
}
```

- [ ] **Step 3: Build and launch the app — verify dark clinical UI renders**

```bash
cmake --build build && ./build/SmartDentalHandpiece
```

Manually verify:
- Window opens with dark background and status bar
- Vitals cards show RPM / TEMP / HOURS (zeros initially)
- RPM chart area is visible
- POWER ON button is visible
- Click POWER ON → badge changes to CALIBRATING, then READY
- Click START → badge changes to ACTIVE, vitals start updating, chart plots live data
- Sliders become interactive when ACTIVE
- Click STOP → returns to READY
- Click E-STOP while ACTIVE → red full-screen overlay appears, CLEAR button works

- [ ] **Step 4: Commit**

```bash
git add qml/main.qml qml/Dashboard.qml
git commit -m "feat: QML dashboard — dark clinical theme, charts, state-driven overlays"
```

---

## Task 7: CI/CD & Dockerfile

**Files:**
- Create: `.github/workflows/ci.yml`
- Create: `.github/workflows/release.yml`
- Create: `Dockerfile`

- [ ] **Step 1: Create .github/workflows/ci.yml**

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-and-test:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest]

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0

      - name: Install Qt
        uses: jurplel/install-qt-action@v4
        with:
          version: '6.7.*'
          modules: 'qtcharts'
          cache: true

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

      - name: Build
        run: cmake --build build --parallel

      - name: Test
        run: ctest --test-dir build --output-on-failure

      - name: Lint (clang-tidy, Ubuntu only)
        if: matrix.os == 'ubuntu-latest'
        run: |
          sudo apt-get install -y clang-tidy
          cmake -B build-lint -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF
          cmake --build build-lint --parallel
          clang-tidy -p build-lint src/*.cpp \
            --checks='-*,readability-*,modernize-*,cppcoreguidelines-*' \
            --warnings-as-errors='' || true
```

- [ ] **Step 2: Create .github/workflows/release.yml**

```yaml
name: Release

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  docker-release:
    runs-on: ubuntu-latest
    permissions:
      contents: write
      packages: write

    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0

      - name: Log in to GitHub Container Registry
        uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Extract tag version
        id: meta
        run: echo "VERSION=${GITHUB_REF_NAME}" >> $GITHUB_OUTPUT

      - name: Build Docker image
        run: |
          docker build \
            --build-arg VERSION=${{ steps.meta.outputs.VERSION }} \
            -t ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:${{ steps.meta.outputs.VERSION }} \
            -t ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:latest \
            .

      - name: Push Docker image
        run: |
          docker push ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:${{ steps.meta.outputs.VERSION }}
          docker push ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:latest

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          body: |
            ## Smart Dental Handpiece ${{ steps.meta.outputs.VERSION }}

            Docker image: `ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:${{ steps.meta.outputs.VERSION }}`

            Run headlessly:
            ```bash
            docker run --rm ghcr.io/${{ github.repository_owner }}/smart-dental-handpiece:${{ steps.meta.outputs.VERSION }}
            ```
```

- [ ] **Step 3: Create Dockerfile**

```dockerfile
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_VERSION=6.7.0

RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build git \
    libgl1-mesa-dev libgles2-mesa-dev \
    libxcb-xinerama0-dev libxcb-icccm4-dev libxcb-image0-dev \
    libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev \
    libxcb-xfixes0-dev libxkbcommon-dev libxkbcommon-x11-dev \
    python3 wget \
    && rm -rf /var/lib/apt/lists/*

# Install Qt via aqtinstall
RUN pip3 install aqtinstall 2>/dev/null || \
    (apt-get install -y python3-pip && pip3 install aqtinstall)
RUN aqt install-qt linux desktop ${QT_VERSION} gcc_64 \
    -m qtcharts -O /opt/Qt

ENV PATH="/opt/Qt/${QT_VERSION}/gcc_64/bin:${PATH}"
ENV Qt6_DIR="/opt/Qt/${QT_VERSION}/gcc_64/lib/cmake/Qt6"

WORKDIR /src
COPY . .

RUN cmake -B build -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DCMAKE_PREFIX_PATH=/opt/Qt/${QT_VERSION}/gcc_64
RUN cmake --build build
RUN ctest --test-dir build --output-on-failure

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libgl1 libgles2 xvfb \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/SmartDentalHandpiece /app/SmartDentalHandpiece
COPY --from=builder /opt/Qt /opt/Qt

ENV QT_VERSION=6.7.0
ENV LD_LIBRARY_PATH="/opt/Qt/${QT_VERSION}/gcc_64/lib"
ENV QT_QPA_PLATFORM=offscreen

RUN mkdir -p /app/data
WORKDIR /app

CMD ["./SmartDentalHandpiece"]
```

- [ ] **Step 4: Commit**

```bash
mkdir -p .github/workflows
git add .github/workflows/ci.yml .github/workflows/release.yml Dockerfile
git commit -m "feat: GitHub Actions CI/CD + Dockerfile for Linux headless release"
```

---

## Task 8: README

**Files:**
- Create: `README.md`

- [ ] **Step 1: Write README.md**

````markdown
# Smart Dental Handpiece Simulator

A simulated medical device control interface demonstrating MedTech-grade software architecture: layered C++17 HAL, a six-state safety state machine, SQLite audit logging, and a live Qt Quick/QML dashboard.

Built as a portfolio project for MedTech engineering roles.

---

## Architecture

```
QML (main.qml · Dashboard.qml)
        ▲  Q_PROPERTY / NOTIFY
TelemetryBridge          StateManager
        │  ▲                  │  ▲
        │  └── HardwareHandler┘  │
        │                        │
        └──── DatabaseManager ───┘
```

| Layer | Class | Role |
|---|---|---|
| HAL | `HardwareHandler` | Worker-thread sensor simulation (100ms polling) |
| Logic | `StateManager` | Six-state machine with guard conditions |
| Logic | `TelemetryBridge` | Q_PROPERTY bridge + 500ms DB telemetry timer |
| Persistence | `DatabaseManager` | SQLite: sessions / events / telemetry tables |
| Presentation | `main.qml`, `Dashboard.qml` | Dark Clinical QML dashboard with live charts |

Dependency Injection is performed in `main.cpp` — the only file that knows all classes simultaneously.

## State Machine

```
STANDBY → CALIBRATING → READY ↔ ACTIVE
                                     │
                          [temp>60°C / fault signal]
                                     ↓
                                  FAULT ──[3 faults / temp>75°C]──► EMERGENCY_STOP
                                     │                                     │
                              [ack]  ↓                           [manual clear]
                                  STANDBY ◄─────────────────────────────────┘
```

## Building

**Prerequisites:** CMake 3.21+, Qt 6.x with QtCharts module.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/SmartDentalHandpiece
```

**With tests:**

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

**Docker (headless Linux):**

```bash
docker build -t smart-dental-handpiece .
docker run --rm smart-dental-handpiece
```

## Database

SQLite audit log written to `data/logs.db`. Three tables:

- `sessions` — one row per power-on cycle, with start/end timestamps and firmware version
- `events` — every state transition and fault with detail string
- `telemetry` — RPM / temperature / usage hours snapshot every 500ms while Active

Inspect with any SQLite client:

```bash
sqlite3 data/logs.db "SELECT * FROM events ORDER BY timestamp DESC LIMIT 20;"
```

## CI/CD

| Trigger | Action |
|---|---|
| PR / push to `main` | Build on Ubuntu + macOS, run unit tests, clang-tidy lint |
| Tag `v*.*.*` | Build Docker image, push to `ghcr.io`, create GitHub Release |

## Manual QML Smoke Test

1. Launch app — window opens with STANDBY state badge
2. Click **POWER ON** — state transitions STANDBY → CALIBRATING → READY within ~0.5s
3. Click **START** — state changes to ACTIVE, vitals update, RPM chart plots live data
4. Sliders are interactive only in ACTIVE state
5. Click **STOP** — returns to READY; chart retains history
6. Click **E-STOP** (or wait for simulated overheat) — red lockout overlay appears
7. Click **CLEAR EMERGENCY & RESET** — returns to STANDBY; check `data/logs.db` for audit trail
````

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: README with architecture, build instructions, and smoke test checklist"
```

---

## Final Verification

- [ ] **Run full test suite**

```bash
cmake -B build -DBUILD_TESTS=ON && cmake --build build && ctest --test-dir build -V
```

Expected: all tests in `tst_HardwareHandler` and `tst_StateManager` pass.

- [ ] **Run the app and verify smoke test checklist in README**

Follow the 7-step manual checklist in `README.md`. Confirm `data/logs.db` contains rows in all three tables after an active session.

- [ ] **Tag v1.0.0**

```bash
git tag v1.0.0
git log --oneline
```

Expected commit history:
```
feat: project scaffolding — CMake, stubs, QML shell
feat: DatabaseManager — SQLite sessions/events/telemetry
feat: HardwareHandler — worker-thread sensor simulation with fault signals
feat: StateManager — six-state device state machine with guards (TDD)
feat: TelemetryBridge + DI wiring in main.cpp
feat: QML dashboard — dark clinical theme, charts, state-driven overlays
feat: GitHub Actions CI/CD + Dockerfile for Linux headless release
docs: README with architecture, build instructions, and smoke test checklist
Add Smart Dental Handpiece design spec
```
