#pragma once
#include "Trigger.h"

// Physischer Button am GPIO des Raspberry Pi.
// Platzhalter: die Implementierung ueber libgpiod folgt in Phase 4,
// sobald der Pi verfuegbar ist.
class TriggerGpio : public Trigger
{
    Q_OBJECT
public:
    TriggerGpio(const QString& chip, int line, QObject* parent = nullptr);

private:
    QString m_chip;
    int m_line;
};
