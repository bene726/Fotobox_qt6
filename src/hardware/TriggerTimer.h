#pragma once
#include "Trigger.h"
#include <QTimer>

// Loest in festem Abstand automatisch aus. Fuer Tests und Demos.
class TriggerTimer : public Trigger
{
    Q_OBJECT
public:
    explicit TriggerTimer(int intervalMs, QObject* parent = nullptr);

private:
    QTimer m_timer;
};
