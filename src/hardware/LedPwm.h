#pragma once
#include "LedController.h"

// LED-Helligkeit ueber Hardware-PWM des Raspberry Pi (sysfs /sys/class/pwm).
// Platzhalter: Implementierung folgt in Phase 4 mit dem echten Pi.
class LedPwm : public LedController
{
    Q_OBJECT
public:
    LedPwm(int chip, int channel, QObject* parent = nullptr);

protected:
    void apply(int percent) override;

private:
    int m_chip;
    int m_channel;
};
