#pragma once
#include <QObject>

class DatabaseManager;

class TelemetryBridge : public QObject {
    Q_OBJECT
public:
    explicit TelemetryBridge(DatabaseManager* db, QObject* parent = nullptr);
};
