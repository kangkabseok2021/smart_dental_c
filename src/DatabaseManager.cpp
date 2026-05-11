// src/DatabaseManager.cpp
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

int DatabaseManager::s_connectionCount = 0;

DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath)
{
    const QString connName = QStringLiteral("sdh_conn_%1").arg(++s_connectionCount);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    m_db.setDatabaseName(dbPath);
}

DatabaseManager::~DatabaseManager() {
    const QString connName = m_db.connectionName();
    m_db.close();
    QSqlDatabase::removeDatabase(connName);
}

bool DatabaseManager::open() {
    if (!m_db.open()) {
        qWarning() << "DatabaseManager: failed to open" << m_dbPath << m_db.lastError().text();
        return false;
    }
    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery q(m_db);
    const QStringList ddl = {
        R"(CREATE TABLE IF NOT EXISTS sessions (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            started_at       TEXT NOT NULL,
            ended_at         TEXT,
            firmware_version TEXT NOT NULL
        ))",
        R"(CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id  INTEGER NOT NULL REFERENCES sessions(id),
            timestamp   TEXT NOT NULL,
            event_type  TEXT NOT NULL,
            from_state  TEXT,
            to_state    TEXT NOT NULL,
            detail      TEXT
        ))",
        R"(CREATE TABLE IF NOT EXISTS telemetry (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id   INTEGER NOT NULL REFERENCES sessions(id),
            timestamp    TEXT NOT NULL,
            rpm          REAL NOT NULL,
            temperature  REAL NOT NULL,
            usage_hours  REAL NOT NULL
        ))"
    };
    for (const auto& stmt : ddl) {
        if (!q.exec(stmt)) {
            qWarning() << "DatabaseManager: DDL failed:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

qint64 DatabaseManager::openSession(const QString& firmwareVersion) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO sessions (started_at, firmware_version) VALUES (?, ?))");
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(firmwareVersion);
    if (!q.exec()) {
        qWarning() << "DatabaseManager::openSession failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toLongLong();
}

void DatabaseManager::closeSession(qint64 sessionId) {
    QSqlQuery q(m_db);
    q.prepare(R"(UPDATE sessions SET ended_at = ? WHERE id = ?)");
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(sessionId);
    q.exec();
}

void DatabaseManager::logEvent(qint64 sessionId,
                                const QString& eventType,
                                const QString& fromState,
                                const QString& toState,
                                const QString& detail)
{
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO events
        (session_id, timestamp, event_type, from_state, to_state, detail)
        VALUES (?, ?, ?, ?, ?, ?))");
    q.addBindValue(sessionId);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(eventType);
    q.addBindValue(fromState.isEmpty() ? QVariant{} : fromState);
    q.addBindValue(toState);
    q.addBindValue(detail.isEmpty() ? QVariant{} : detail);
    q.exec();
}

void DatabaseManager::logTelemetry(qint64 sessionId,
                                    double rpm,
                                    double temperature,
                                    double usageHours)
{
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO telemetry
        (session_id, timestamp, rpm, temperature, usage_hours)
        VALUES (?, ?, ?, ?, ?))");
    q.addBindValue(sessionId);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(rpm);
    q.addBindValue(temperature);
    q.addBindValue(usageHours);
    q.exec();
}
