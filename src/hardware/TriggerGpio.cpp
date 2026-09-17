#include "TriggerGpio.h"

#include <QDebug>

TriggerGpio::TriggerGpio(const QString& chip, int line, QObject* parent)
    : Trigger(parent)
    , m_chip(chip)
    , m_line(line)
{
    qWarning() << "GPIO-Trigger noch nicht implementiert (" << m_chip << "Line" << m_line
               << "). Es wird kein Foto per Button ausgeloest.";
}
