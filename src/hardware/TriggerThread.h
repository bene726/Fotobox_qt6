#pragma once
#include <QThread>

class TriggerThread : public QThread
{
    Q_OBJECT
public:
    void run() override;

signals:
    void triggered();
};
