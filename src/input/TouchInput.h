#pragma once
#include <QObject>

class TouchInput : public QObject
{
    Q_OBJECT
public:
    explicit TouchInput(QObject* parent = nullptr);
signals:
    void touched();
};
