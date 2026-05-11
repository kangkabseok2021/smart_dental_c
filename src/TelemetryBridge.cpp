#include "TelemetryBridge.h"

TelemetryBridge::TelemetryBridge(DatabaseManager* db, QObject* parent)
    : QObject(parent) { Q_UNUSED(db) }
