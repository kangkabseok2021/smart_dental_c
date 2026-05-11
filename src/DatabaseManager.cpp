#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent)
    : QObject(parent) { Q_UNUSED(dbPath) }
