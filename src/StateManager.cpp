#include "StateManager.h"

StateManager::StateManager(DatabaseManager* db, QObject* parent)
    : QObject(parent) { Q_UNUSED(db) }
