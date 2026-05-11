// src/main.cpp
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QQuickStyle>

#include "HardwareHandler.h"
#include "StateManager.h"
#include "TelemetryBridge.h"
#include "DatabaseManager.h"

int main(int argc, char* argv[]) {
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SmartDentalHandpiece"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    QDir().mkpath(QStringLiteral("data"));

    DatabaseManager db(QStringLiteral("data/logs.db"));
    if (!db.open())
        qWarning() << "Failed to open database — logging disabled";

    HardwareHandler hardware;
    StateManager    stateManager(&db);
    TelemetryBridge telemetryBridge(&db);

    QObject::connect(&hardware, &HardwareHandler::sensorUpdate,
                     &stateManager, &StateManager::onSensorUpdate);
    QObject::connect(&hardware, &HardwareHandler::faultDetected,
                     &stateManager, &StateManager::onFaultDetected);

    QObject::connect(&hardware, &HardwareHandler::sensorUpdate,
                     &telemetryBridge, &TelemetryBridge::onSensorUpdate);

    QObject::connect(&stateManager, &StateManager::stateChanged,
                     &telemetryBridge, &TelemetryBridge::onStateChanged);
    QObject::connect(&stateManager, &StateManager::sessionStarted,
                     &telemetryBridge, &TelemetryBridge::onSessionStarted);

    hardware.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("stateManager"),    &stateManager);
    engine.rootContext()->setContextProperty(QStringLiteral("telemetryBridge"), &telemetryBridge);
    engine.rootContext()->setContextProperty(QStringLiteral("appVersion"),
                                             QStringLiteral(APP_VERSION));

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
