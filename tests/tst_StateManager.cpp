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
