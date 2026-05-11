// src/HardwareHandler.h
#pragma once
#include <QObject>
#include <QThread>

class QTimer;

class HardwarePoller : public QObject {
    Q_OBJECT
public:
    explicit HardwarePoller(QObject* parent = nullptr);
public slots:
    void start();
    void stop();
signals:
    void sensorUpdate(double rpm, double temperature, double usageHours);
    void faultDetected(const QString& type, const QString& detail);
private slots:
    void poll();
private:
    QTimer* m_timer{nullptr};
    double  m_usageHours{0.0};
    int     m_tick{0};
};

class HardwareHandler : public QObject {
    Q_OBJECT
public:
    explicit HardwareHandler(QObject* parent = nullptr);
    ~HardwareHandler() override;

    void start();
    void stop();

signals:
    void sensorUpdate(double rpm, double temperature, double usageHours);
    void faultDetected(const QString& type, const QString& detail);

private:
    QThread         m_workerThread;
    HardwarePoller* m_poller;
};
