#include "LedController.h"

#include <QDebug>

void LedController::setBrightness(int percent)
{
    percent = qBound(0, percent, 100);
    if (percent == m_brightness)
        return;
    m_brightness = percent;
    apply(percent);
    emit brightnessChanged(percent);
}

void LedDummy::apply(int percent)
{
    // Keine Hardware, keine Ausgabe: der Slider wuerde das Log sonst fluten.
    Q_UNUSED(percent);
}
