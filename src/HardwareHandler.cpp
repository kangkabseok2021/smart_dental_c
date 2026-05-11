// src/HardwareHandler.cpp
#include "HardwareHandler.h"
#include <QTimer>
#include <cmath>
#include <random>

namespace {
    std::mt19937& rng() {
        static std::mt19937 gen{std::random_device{}()};
        return gen;
    }
}

// ── HardwarePoller ──────────────────────────────────────────────────────────

HardwarePoller::HardwarePoller(QObject* parent) : QObject(parent) {}

void HardwarePoller::start() {
    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &HardwarePoller::poll);
    m_timer->start();
}

void HardwarePoller::stop() {
    if (m_timer) m_timer->stop();
}

void HardwarePoller::poll() {
    ++m_tick;
    m_usageHours += 100.0 / 3'600'000.0;

    // Simulate sensor dropout (SENSOR_NAN): ~1 in 500 ticks
    std::uniform_int_distribution<int> nanDist(1, 500);
    if (nanDist(rng()) == 1) {
        emit faultDetected(QStringLiteral("SENSOR_NAN"),
                           QStringLiteral("RPM sensor dropout"));
        return;  // no sensorUpdate emitted
    }

    // Simulate motor stall: ~1 in 200 ticks
    std::uniform_int_distribution<int> stallDist(1, 200);
    if (stallDist(rng()) == 1) {
        emit faultDetected(QStringLiteral("MOTOR_STALL"),
                           QStringLiteral("RPM=0 detected for >500ms"));
    }

    std::normal_distribution<double> rpmNoise(0.0, 400.0);
    std::normal_distribution<double> tempNoise(0.0, 0.15);

    const double baseRpm = 30'000.0;
    double rpm = baseRpm
               + 4'000.0 * std::sin(m_tick * 0.04)
               + rpmNoise(rng());
    rpm = std::clamp(rpm, 0.0, 400'000.0);

    double temperature = 36.5
                       + (rpm / baseRpm) * 1.8
                       + 0.0005 * m_tick
                       + tempNoise(rng());
    temperature = std::clamp(temperature, 15.0, 100.0);

    emit sensorUpdate(rpm, temperature, m_usageHours);
}

// ── HardwareHandler ─────────────────────────────────────────────────────────

HardwareHandler::HardwareHandler(QObject* parent) : QObject(parent) {
    m_poller = new HardwarePoller;
    m_poller->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::started,
            m_poller, &HardwarePoller::start);
    connect(m_poller, &HardwarePoller::sensorUpdate,
            this,    &HardwareHandler::sensorUpdate);
    connect(m_poller, &HardwarePoller::faultDetected,
            this,    &HardwareHandler::faultDetected);
}

HardwareHandler::~HardwareHandler() {
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_poller;
}

void HardwareHandler::start() {
    m_workerThread.start();
}

void HardwareHandler::stop() {
    QMetaObject::invokeMethod(m_poller, "stop", Qt::QueuedConnection);
}
