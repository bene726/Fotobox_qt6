#pragma once
#include "LedController.h"

#include <QString>

// LED-Helligkeit ueber die Hardware-PWM des Raspberry Pi (/sys/class/pwm).
//
// Voraussetzung ist ein passender Overlay-Eintrag in /boot/firmware/config.txt,
// z.B. "dtoverlay=pwm,pin=18,func=2" oder "dtoverlay=pwm-2chan".
// Der angesteuerte Pin schaltet einen Transistor, der das LED-Band versorgt.
class LedPwm : public LedController
{
    Q_OBJECT
public:
    // chip: Nummer von /sys/class/pwm/pwmchipN, oder -1 fuer automatische Suche
    LedPwm(int chip, int channel, int frequencyHz, QObject* parent = nullptr);
    ~LedPwm() override;

    bool isReady() const { return !m_dir.isEmpty(); }

protected:
    void apply(int percent) override;

private:
    bool setup(int chip, int channel);
    static bool writeFile(const QString& path, const QString& value, QString* error = nullptr);
    static int findChip(int channel);

    QString m_dir;        // .../pwmchipN/pwmM
    long m_periodNs = 0;
    int m_frequencyHz = 1000;
};
