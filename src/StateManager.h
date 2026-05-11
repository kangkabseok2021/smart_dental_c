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

    Q_INVOKABLE void startDevice();
    Q_INVOKABLE void startProcedure();
    Q_INVOKABLE void stopProcedure();
    Q_INVOKABLE void acknowledgeFault();
    Q_INVOKABLE void clearEmergency();

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

    int m_validCalibrationTicks{0};
    static constexpr int kCalibrationTicks{3};

    int m_sessionFaultCount{0};
    static constexpr int kMaxFaults{3};

    qint64  m_sessionId{-1};
    bool    m_sessionActive{false};
    QString m_faultDetail;
};
