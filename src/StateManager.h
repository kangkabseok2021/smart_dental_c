#pragma once
#include <QObject>

class DatabaseManager;

class StateManager : public QObject {
    Q_OBJECT
public:
    explicit StateManager(DatabaseManager* db, QObject* parent = nullptr);
};
