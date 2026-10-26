#include "TriggerGpio.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSocketNotifier>

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
{
    QString path = (chip.isEmpty() || chip == "auto") ? findHeaderChip() : chip;
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
    // Alle anstehenden Ereignisse abholen, sonst meldet sich der Notifier erneut.
    gpio_v2_line_event ev{};
    int handled = 0;
    while (::read(m_fd, &ev, sizeof(ev)) == sizeof(ev))
        handled++;

    if (handled == 0)
        return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_softwareDebounce && m_debounceMs > 0 && now - m_lastEventMs < m_debounceMs)
        return;
    m_lastEventMs = now;

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
