#include "LedPwm.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
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
    // Deshalb den ersten Chip nehmen, der genug Kanaele hat.
    const QStringList chips = QDir("/sys/class/pwm").entryList({"pwmchip*"}, QDir::Dirs, QDir::Name);
    for (const QString& c : chips) {
        QFile f("/sys/class/pwm/" + c + "/npwm");
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const int n = f.readAll().trimmed().toInt();
        if (n > channel)
            return QStringView{c}.mid(7).toInt();   // "pwmchip" ist 7 Zeichen, dann die Nummer
    }
    return -1;
}

LedPwm::LedPwm(int chip, int channel, int frequencyHz, QObject* parent)
    : LedController(parent)
    , m_frequencyHz(qBound(50, frequencyHz, 100000))
{
    if (chip < 0) {
        chip = findChip(channel);
        if (chip < 0) {
            qWarning() << "Keine PWM gefunden. In /boot/firmware/config.txt fehlt vermutlich"
                       << "ein Eintrag wie 'dtoverlay=pwm,pin=18,func=2'."
                       << "Die LED bleibt dunkel.";
            return;
        }
        qInfo() << "PWM-Chip automatisch gewaehlt: pwmchip" << chip;
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
            qWarning() << "PWM-Kanal" << channel << "nicht zu aktivieren:" << err
                       << "- gehoert der Benutzer zur Gruppe 'gpio'?";
            return false;
        }
        // udev legt die Dateien asynchron an und setzt die Rechte.
        QElapsedTimer t; t.start();
        while (!QFile::exists(dir + "/period") && t.elapsed() < 2000)
            QThread::msleep(50);
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
