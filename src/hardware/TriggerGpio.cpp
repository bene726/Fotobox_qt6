#include "TriggerGpio.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSocketNotifier>
#include <QTimer>

#ifdef __linux__
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef __linux__

namespace {
// Ab so vielen Flanken je Sekunde war es kein Finger mehr.
constexpr int kBurstPerSecond = 2000;
// So lange nach einem gemeldeten Druck gilt alles als Prellen.
constexpr int kMinLockoutMs = 400;
} // namespace

QString TriggerGpio::findHeaderChip()
{
    // Der Chip mit den Header-Pins heisst auf dem Pi 4 "pinctrl-bcm2711" und
    // auf dem Pi 5 "pinctrl-rp1". Die Nummer von /dev/gpiochipN wechselt je
    // nach Kernel, deshalb wird nach dem Label gesucht.
    const QStringList names = QDir("/dev").entryList({"gpiochip*"}, QDir::System | QDir::Files,
                                                     QDir::Name);
    for (const QString& name : names) {
        const QString path = "/dev/" + name;
        const int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            continue;
        gpiochip_info info{};
        const bool ok = ::ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info) >= 0;
        ::close(fd);
        if (ok && QString::fromLocal8Bit(info.label).startsWith("pinctrl-")) {
            qInfo() << "GPIO-Chip gefunden:" << path << "(" << info.label << ","
                    << info.lines << "Leitungen )";
            return path;
        }
    }
    return {};
}

TriggerGpio::TriggerGpio(const QString& chip, int line, const QString& pull, int debounceMs,
                         QObject* parent)
    : Trigger(parent)
    , m_debounceMs(qMax(0, debounceMs))
    , m_lockoutMs(qMax(kMinLockoutMs, qMax(0, debounceMs)))
{
    QString path = (chip.isEmpty() || chip == "auto") ? findHeaderChip() : chip;
    // Eine blosse Zahl ist als gpiochipN gemeint, nicht als /dev/N.
    bool nurZiffern = false;
    path.toInt(&nurZiffern);
    if (nurZiffern)
        path = "gpiochip" + path;
    if (!path.startsWith('/'))
        path = "/dev/" + path;
    if (path == "/dev/") {
        qWarning() << "Kein GPIO-Chip gefunden. Der Taster bleibt ohne Funktion.";
        return;
    }

    const int chipFd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
    if (chipFd < 0) {
        qWarning() << "GPIO-Chip" << path << "nicht zu oeffnen:" << ::strerror(errno)
                   << "- gehoert der Benutzer zur Gruppe 'gpio'?";
        return;
    }

    const bool pullUp = (pull != "down" && pull != "none");

    gpio_v2_line_request req{};
    req.offsets[0] = static_cast<__u32>(line);
    req.num_lines = 1;
    std::snprintf(req.consumer, sizeof(req.consumer), "fotobox");
    req.config.flags = GPIO_V2_LINE_FLAG_INPUT;
    if (pull == "up")
        req.config.flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
    else if (pull == "down")
        req.config.flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN;
    else
        req.config.flags |= GPIO_V2_LINE_FLAG_BIAS_DISABLED;
    // Pull-up: Druecken zieht die Leitung auf Masse -> fallende Flanke.
    req.config.flags |= pullUp ? GPIO_V2_LINE_FLAG_EDGE_FALLING
                               : GPIO_V2_LINE_FLAG_EDGE_RISING;

    // Entprellen moeglichst im Kernel erledigen.
    if (m_debounceMs > 0) {
        req.config.num_attrs = 1;
        req.config.attrs[0].mask = 1ULL;
        req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
        req.config.attrs[0].attr.debounce_period_us = static_cast<__u32>(m_debounceMs * 1000);
    }

    if (::ioctl(chipFd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
        const int err = errno;
        // Nicht jeder Treiber kann entprellen: ohne dieses Attribut erneut versuchen.
        if (req.config.num_attrs > 0) {
            qInfo() << "Kernel entprellt diese Leitung nicht (" << ::strerror(err)
                    << ") - Entprellen erfolgt in der Anwendung.";
            req.config.num_attrs = 0;
            m_softwareDebounce = true;
            if (::ioctl(chipFd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
                qWarning() << "GPIO-Leitung" << line << "nicht anzufordern:" << ::strerror(errno);
                ::close(chipFd);
                return;
            }
        } else {
            qWarning() << "GPIO-Leitung" << line << "nicht anzufordern:" << ::strerror(err);
            ::close(chipFd);
            return;
        }
    }
    ::close(chipFd);

    m_fd = req.fd;

    // Entscheidend: der Ereignis-Deskriptor ist von Haus aus blockierend.
    // Die Leseschleife unten holt Ereignisse, bis keines mehr da ist; genau
    // dann wuerde read() warten statt zurueckzukehren und die Oberflaeche
    // einfrieren. Ein einziger Tastendruck genuegt dafuer.
    const int flags = ::fcntl(m_fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        qWarning() << "GPIO-Leitung nicht auf nicht-blockierend umstellbar:" << ::strerror(errno)
                   << "- der Taster bleibt deaktiviert, sonst friert die Anzeige ein.";
        ::close(m_fd);
        m_fd = -1;
        return;
    }

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &TriggerGpio::onEvent);

    qInfo() << "Taster aktiv:" << path << "Leitung" << line << ", Pull" << pull
            << ", Flanke" << (pullUp ? "fallend" : "steigend")
            << ", Entprellen" << m_debounceMs << "ms"
            << (m_softwareDebounce ? "(Anwendung)" : "(Kernel)");
}

TriggerGpio::~TriggerGpio()
{
    if (m_notifier)
        m_notifier->setEnabled(false);
    if (m_fd >= 0)
        ::close(m_fd);
}

void TriggerGpio::onEvent()
{
    // Anstehende Ereignisse abholen. Der Deskriptor ist nicht-blockierend, der
    // letzte Leseversuch liefert daher einen Fehler statt zu warten. Zusaetzlich
    // begrenzt, damit eine staendig wackelnde Leitung die Schleife nicht
    // beliebig lange beschaeftigt; der Rest folgt beim naechsten Durchlauf.
    gpio_v2_line_event ev{};
    int handled = 0;
    while (handled < 32 && ::read(m_fd, &ev, sizeof(ev)) == sizeof(ev))
        handled++;

    if (handled == 0)
        return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Flankensturm: lieber kurz taub stellen als unbedienbar werden. Gezaehlt
    // werden alle Flanken, auch die aus dem Nachlauf unten; ein Finger bringt
    // selbst an einem schlecht entprellten Taster keine tausend pro Sekunde
    // zusammen, eine einstreuende oder scheuernde Leitung schon.
    if (now - m_burstStart > 1000) {
        m_burstStart = now;
        m_burst = 0;
    }
    m_burst += handled;
    if (m_burst > kBurstPerSecond) {
        // Nur gelegentlich melden, sonst besteht das Log aus dieser Zeile.
        if (now - m_lastBurstWarnMs > 30000) {
            qWarning() << "Mehr als" << m_burst << "Flanken je Sekunde an der Taster-Leitung."
                       << "Verkabelung pruefen. Eingang wird 2 s stummgeschaltet.";
            m_lastBurstWarnMs = now;
        }
        if (m_notifier)
            m_notifier->setEnabled(false);
        QTimer::singleShot(2000, this, [this] {
            m_burst = 0;
            m_burstStart = 0;
            if (m_notifier)
                m_notifier->setEnabled(true);
        });
        return;
    }

    // Nachlauf eines Tastendrucks. Die Kontakte prellen, und auf einem langen
    // Kabel kommen dabei leicht einige hundert Flanken zusammen - gemessen mit
    // --gpio-watch. Sie gehoeren zum schon gemeldeten Druck und werden nur noch
    // weggelesen, damit nicht gleich die naechste Aufnahme startet.
    if (m_lastTriggerMs > 0 && now - m_lastTriggerMs < m_lockoutMs) {
        m_swallowed += handled;
        return;
    }

    if (m_softwareDebounce && m_debounceMs > 0 && now - m_lastEventMs < m_debounceMs)
        return;
    m_lastEventMs = now;

    m_lastTriggerMs = now;
    m_swallowed = 0;
    // Hilft beim Beurteilen der Verkabelung, ohne das Log zu fluten: eine
    // Zeile je Tastendruck, und nur wenn es auffaellig viel war.
    QTimer::singleShot(m_lockoutMs, this, [this] {
        if (m_swallowed > 50)
            qInfo() << "Taster prellt:" << m_swallowed << "Flanken innerhalb von"
                    << m_lockoutMs << "ms nach dem Druck unterdrueckt.";
        m_swallowed = 0;
    });

    emit triggered();
}

#else  // kein Linux: nur ein Platzhalter, damit das Projekt auf dem Host baut

QString TriggerGpio::findHeaderChip()
{
    return {};
}

TriggerGpio::TriggerGpio(const QString& chip, int line, const QString& pull, int debounceMs,
                         QObject* parent)
    : Trigger(parent)
    , m_debounceMs(debounceMs)
{
    Q_UNUSED(chip); Q_UNUSED(line); Q_UNUSED(pull);
    qWarning() << "GPIO gibt es nur unter Linux. Der Taster bleibt ohne Funktion.";
}

TriggerGpio::~TriggerGpio() = default;

void TriggerGpio::onEvent() {}

#endif
