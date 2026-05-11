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

    if (m_db && m_sessionId != -1)
        m_db->logEvent(m_sessionId, QStringLiteral("STATE_CHANGE"), from, to, detail);

    emit stateChanged(next);
}

void StateManager::startDevice() {
    if (m_state != DeviceState::STANDBY) return;

    m_validCalibrationTicks = 0;

    if (!m_sessionActive) {
        m_sessionActive = true;
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
    if (m_db && m_sessionId != -1)
        m_db->closeSession(m_sessionId);
    m_sessionId = -1;
    m_sessionActive = false;
    m_sessionFaultCount = 0;
    m_faultDetail.clear();
    emit faultDetailChanged();
    transitionTo(DeviceState::STANDBY);
}

void StateManager::onSensorUpdate(double /*rpm*/, double temperature, double /*usageHours*/) {
    switch (m_state) {
    case DeviceState::CALIBRATING:
        if (++m_validCalibrationTicks >= kCalibrationTicks)
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

    if (m_sessionFaultCount >= kMaxFaults)
        transitionTo(DeviceState::EMERGENCY_STOP, m_faultDetail);
    else
        transitionTo(DeviceState::FAULT, m_faultDetail);
}
