#include "GpioScan.h"
#include "TriggerGpio.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSocketNotifier>
#include <QTextStream>

#ifdef __linux__
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {
QTextStream& out()
{
    static QTextStream s(stdout);
    return s;
}
}

#ifdef __linux__

namespace {

// Pin am 40-poligen Header zur BCM-Nummer (nur zur Anzeige).
int headerPin(int bcm)
{
    static const int map[41] = {
        0, 0, 0, 2, 0, 3, 0, 4, 14, 0, 15, 17, 18, 27, 0, 22, 23, 0, 24, 10,
        0, 9, 25, 11, 8, 0, 7, 0, 1, 5, 0, 6, 12, 13, 0, 19, 16, 26, 20, 0, 21
    };
    for (int pin = 1; pin <= 40; ++pin)
        if (map[pin] == bcm && bcm != 0)
            return pin;
    return 0;
}

struct Watched {
    int fd;
    int line;
};

} // namespace

int GpioScan::run(bool watch)
{
    const QString path = TriggerGpio::findHeaderChip();
    if (path.isEmpty()) {
        out() << "Kein GPIO-Chip gefunden.\n";
        out().flush();
        return 1;
    }

    const int chipFd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_CLOEXEC);
    if (chipFd < 0) {
        out() << "Chip " << path << " nicht zu oeffnen: " << ::strerror(errno)
              << "\nGehoert der Benutzer zur Gruppe 'gpio'?\n";
        out().flush();
        return 1;
    }

    gpiochip_info info{};
    if (::ioctl(chipFd, GPIO_GET_CHIPINFO_IOCTL, &info) < 0) {
        out() << "Chip-Infos nicht lesbar: " << ::strerror(errno) << "\n";
        out().flush();
        ::close(chipFd);
        return 1;
    }

    out() << "Chip " << path << "  (" << info.label << ", " << info.lines << " Leitungen)\n\n";
    out() << "BCM  Pin  Zustand  Name                 belegt von\n";
    out() << "---  ---  -------  -------------------  --------------------\n";

    QList<int> free;
    for (__u32 i = 0; i < info.lines; ++i) {
        gpio_v2_line_info li{};
        li.offset = i;
        if (::ioctl(chipFd, GPIO_V2_GET_LINEINFO_IOCTL, &li) < 0)
            continue;
        const bool used = li.flags & GPIO_V2_LINE_FLAG_USED;
        const int pin = headerPin(static_cast<int>(i));
        out() << QString("%1  %2  %3  %4  %5\n")
                     .arg(i, 3)
                     .arg(pin ? QString::number(pin) : QString("-"), 3)
                     .arg((li.flags & GPIO_V2_LINE_FLAG_OUTPUT) ? "Ausgang" : "Eingang", 7)
                     .arg(QString::fromLocal8Bit(li.name), -19)
                     .arg(used ? QString::fromLocal8Bit(li.consumer) : QString("frei"));
        if (!used && pin > 0)
            free << static_cast<int>(i);
    }
    out() << "\n";
    out().flush();

    if (!watch) {
        ::close(chipFd);
        return 0;
    }

    // Alle freien Header-Leitungen auf Flanken beobachten.
    auto* watched = new QList<Watched>();
    for (int line : free) {
        gpio_v2_line_request req{};
        req.offsets[0] = static_cast<__u32>(line);
        req.num_lines = 1;
        std::snprintf(req.consumer, sizeof(req.consumer), "fotobox-scan");
        req.config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_BIAS_PULL_UP
                           | GPIO_V2_LINE_FLAG_EDGE_FALLING | GPIO_V2_LINE_FLAG_EDGE_RISING;
        if (::ioctl(chipFd, GPIO_V2_GET_LINE_IOCTL, &req) < 0)
            continue;
        watched->append({req.fd, line});
    }
    ::close(chipFd);

    if (watched->isEmpty()) {
        out() << "Keine Leitung konnte beobachtet werden.\n";
        out().flush();
        return 1;
    }

    out() << "Beobachte " << watched->size() << " Leitungen mit internem Pull-up.\n"
          << "Jetzt den Taster druecken. Beenden mit Strg+C.\n\n";
    out().flush();

    for (const Watched& w : *watched) {
        auto* n = new QSocketNotifier(w.fd, QSocketNotifier::Read, qApp);
        const int line = w.line;
        QObject::connect(n, &QSocketNotifier::activated, qApp, [line, fd = w.fd]() {
            gpio_v2_line_event ev{};
            while (::read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                const char* kind = (ev.id == GPIO_V2_LINE_EVENT_FALLING_EDGE)
                                       ? "fallend (gedrueckt)" : "steigend (losgelassen)";
                out() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                      << "  BCM " << line << " (Pin " << headerPin(line) << ")  " << kind << "\n";
                out().flush();
            }
        });
    }

    return qApp->exec();
}

#else

int GpioScan::run(bool)
{
    out() << "GPIO gibt es nur unter Linux.\n";
    out().flush();
    return 1;
}

#endif
