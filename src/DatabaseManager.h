#pragma once
#include <QObject>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(const QString& dbPath, QObject* parent = nullptr);
};
