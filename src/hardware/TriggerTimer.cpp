#include "TriggerTimer.h"

TriggerTimer::TriggerTimer(int intervalMs, QObject* parent)
    : Trigger(parent)
{
    m_timer.setInterval(intervalMs);
    connect(&m_timer, &QTimer::timeout, this, &Trigger::triggered);
    m_timer.start();
}
