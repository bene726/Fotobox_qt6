#include "LedPwm.h"

#include <QDebug>

LedPwm::LedPwm(int chip, int channel, QObject* parent)
    : LedController(parent)
    , m_chip(chip)
    , m_channel(channel)
{
    qWarning() << "LED-PWM noch nicht implementiert (pwmchip" << m_chip
               << "Kanal" << m_channel << ")";
}

void LedPwm::apply(int percent)
{
    qInfo() << "LED (pwm, Platzhalter) Helligkeit:" << percent << "%";
}
