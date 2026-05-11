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
    if (m_db && m_sessionId != -1)
        m_db->logTelemetry(m_sessionId, m_rpm, m_temperature, m_usageHours);
}
