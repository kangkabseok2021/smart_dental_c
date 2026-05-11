#pragma once
#include <QObject>

class HardwareHandler : public QObject {
    Q_OBJECT
public:
    explicit HardwareHandler(QObject* parent = nullptr);
};
