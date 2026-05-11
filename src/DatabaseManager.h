// src/DatabaseManager.h
#pragma once
#include <QObject>
#include <QSqlDatabase>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(const QString& dbPath, QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool open();

    qint64 openSession(const QString& firmwareVersion);
    void closeSession(qint64 sessionId);
    void logEvent(qint64 sessionId,
                  const QString& eventType,
                  const QString& fromState,
                  const QString& toState,
                  const QString& detail = {});
    void logTelemetry(qint64 sessionId,
                      double rpm,
                      double temperature,
                      double usageHours);

private:
    bool createTables();

    QSqlDatabase m_db;
    QString m_dbPath;
    static int s_connectionCount;
};
