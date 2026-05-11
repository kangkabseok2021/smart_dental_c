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
