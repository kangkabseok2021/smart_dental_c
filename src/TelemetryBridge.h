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
