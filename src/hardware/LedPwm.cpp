#include "LedPwm.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFile>
#include <QThread>

bool LedPwm::writeFile(const QString& path, const QString& value, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    const QByteArray data = value.toLatin1();
    const bool ok = f.write(data) == data.size();
    if (!ok && error)
        *error = f.errorString();
    return ok;
}

int LedPwm::findChip(int channel)
{
    // Die Nummer von pwmchipN haengt vom Kernel und den geladenen Overlays ab.
    // Es kann mehrere geben, etwa den Luefter des Pi 5. Deshalb werden alle
    // protokolliert, damit die Wahl nachvollziehbar ist und notfalls per
    // "ledPwmChip" festgelegt werden kann.
    const QStringList chips = QDir("/sys/class/pwm").entryList({"pwmchip*"}, QDir::Dirs, QDir::Name);

    // Auf dem Pi 5 gibt es mehrere: der Hardware-Block des RP1 (oft fuer den
    // Luefter) und der vom Overlay pwm-pio angelegte. Gesucht ist der zweite,
    // sonst wuerde die Fotobox am falschen Geraet drehen.
    int ersterPassender = -1, pio = -1;
    QStringList zeilen;
    for (const QString& c : chips) {
        QFile f("/sys/class/pwm/" + c + "/npwm");
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const int n = f.readAll().trimmed().toInt();
        const QString geraet =
            QFileInfo(QFileInfo("/sys/class/pwm/" + c + "/device").symLinkTarget()).fileName();
        const int num = QStringView{c}.mid(7).toInt();   // "pwmchip" ist 7 Zeichen
        if (n > channel) {
            if (ersterPassender < 0)
                ersterPassender = num;
            if (pio < 0 && geraet.contains("pwm_pio"))
                pio = num;
        }
        zeilen << QString("  %1: %2 Kanaele, %3").arg(c).arg(n).arg(geraet.isEmpty() ? "?" : geraet);
    }

    const int chosen = pio >= 0 ? pio : ersterPassender;
    for (int i = 0; i < zeilen.size(); ++i) {
        const bool gewaehlt = zeilen.at(i).startsWith(QString("  pwmchip%1:").arg(chosen));
        qInfo().noquote() << zeilen.at(i) + (gewaehlt ? "   <- gewaehlt" : "");
    }
    if (chips.isEmpty())
        qWarning() << "Unter /sys/class/pwm liegt nichts. Fehlt der dtoverlay-Eintrag?";
    else if (chosen >= 0 && pio < 0)
        qInfo() << "Kein pwm_pio-Geraet gefunden. Passt der gewaehlte Chip nicht,"
                << "mit \"ledPwmChip\" festlegen.";
    return chosen;
}

LedPwm::LedPwm(int chip, int channel, int frequencyHz, QObject* parent)
    : LedController(parent)
    , m_frequencyHz(qBound(50, frequencyHz, 100000))
{
    if (chip < 0) {
        qInfo() << "Suche PWM-Chip:";
        chip = findChip(channel);
        if (chip < 0) {
            qWarning().noquote()
                << "Keine PWM gefunden. In /boot/firmware/config.txt fehlt vermutlich ein\n"
                   "         Eintrag, auf dem Pi 5 z.B. 'dtoverlay=pwm-pio,gpio=12'.\n"
                   "         Die LED bleibt dunkel.";
            return;
        }
    }
    if (!setup(chip, channel))
        m_dir.clear();
}

bool LedPwm::setup(int chip, int channel)
{
    const QString base = QString("/sys/class/pwm/pwmchip%1").arg(chip);
    const QString dir = QString("%1/pwm%2").arg(base).arg(channel);

    if (!QDir(dir).exists()) {
        QString err;
        if (!writeFile(base + "/export", QString::number(channel), &err)) {
            // "Device or resource busy" heisst nur: schon exportiert.
            if (!QDir(dir).exists()) {
                qWarning() << "PWM-Kanal" << channel << "nicht zu aktivieren:" << err
                           << "- gehoert der Benutzer zur Gruppe 'gpio'?";
                return false;
            }
        }
    }

    // Nach dem Exportieren legt der Kernel die Dateien an, die Rechte setzt
    // aber udev, und zwar kurz danach. Wer nur auf das Vorhandensein wartet,
    // schreibt in genau dieses Fenster hinein und scheitert.
    {
        QElapsedTimer t;
        t.start();
        forever {
            const QFileInfo fi(dir + "/period");
            if (fi.exists() && fi.isWritable())
                break;
            if (t.elapsed() > 3000) {
                qWarning() << "PWM-Dateien in" << dir
                           << (fi.exists() ? "sind nicht beschreibbar - Gruppe 'gpio' pruefen."
                                           : "sind nicht aufgetaucht.");
                return false;
            }
            QThread::msleep(50);
        }
    }

    m_periodNs = 1000000000L / m_frequencyHz;
    QString err;
    // Einschaltdauer vor der Periode auf 0 setzen, sonst lehnt der Treiber ab.
    writeFile(dir + "/duty_cycle", "0");
    if (!writeFile(dir + "/period", QString::number(m_periodNs), &err)) {
        qWarning() << "PWM-Periode nicht zu setzen:" << err;
        return false;
    }
    writeFile(dir + "/polarity", "normal");   // nicht jeder Treiber kann das
    if (!writeFile(dir + "/enable", "1", &err)) {
        qWarning() << "PWM nicht zu starten:" << err;
        return false;
    }

    m_dir = dir;
    qInfo() << "LED-PWM aktiv:" << m_dir << "," << m_frequencyHz << "Hz";
    return true;
}

void LedPwm::apply(int percent)
{
    if (m_dir.isEmpty())
        return;
    const long duty = m_periodNs * qBound(0, percent, 100) / 100;
    QString err;
    if (!writeFile(m_dir + "/duty_cycle", QString::number(duty), &err))
        qWarning() << "LED-Helligkeit nicht zu setzen:" << err;
}

LedPwm::~LedPwm()
{
    if (m_dir.isEmpty())
        return;
    // LED ausschalten, Kanal aber aktiviert lassen.
    writeFile(m_dir + "/duty_cycle", "0");
    writeFile(m_dir + "/enable", "0");
}
