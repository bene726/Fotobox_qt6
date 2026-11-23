#include "GpioScan.h"
#include "TriggerGpio.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QSocketNotifier>
#include <QTextStream>
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

// Eine Flanke, so wie der Kernel sie gemeldet hat.
struct Edge {
    quint64 ns = 0;        // Zeitstempel des Kernels, nicht der Ausgabe
    quint64 gapNs = 0;     // Abstand zur vorigen Flanke derselben Leitung
    bool falling = false;
};

// Was auf einer Leitung seit dem letzten Bericht angefallen ist.
struct LineStat {
    QList<Edge> edges;
    quint64 lastNs = 0;
    bool haveLast = false;
};

QString nsText(quint64 ns)
{
    if (ns == 0)
        return "0";
    if (ns < 1000000ULL)
        return QString::number(ns / 1000.0, 'f', 1) + " us";
    if (ns < 1000000000ULL)
        return QString::number(ns / 1000000.0, 'f', ns < 10000000ULL ? 2 : 0) + " ms";
    return QString::number(ns / 1000000000.0, 'f', 2) + " s";
}

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
        // Nicht-blockierend, sonst wartet der letzte Leseversuch (siehe TriggerGpio).
        const int flags = ::fcntl(req.fd, F_GETFL, 0);
        if (flags < 0 || ::fcntl(req.fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            ::close(req.fd);
            continue;
        }
        watched->append({req.fd, line});
    }
    ::close(chipFd);

    if (watched->isEmpty()) {
        out() << "Keine Leitung konnte beobachtet werden.\n";
        out().flush();
        return 1;
    }

    out() << "Beobachte " << watched->size() << " Leitungen mit internem Pull-up.\n"
          << "Diese Anzeige entprellt absichtlich nicht.\n"
          << "Jetzt den Taster druecken. Beenden mit Strg+C.\n\n";
    out().flush();

    auto* stats = new QHash<int, LineStat>();

    for (const Watched& w : *watched) {
        auto* notifier = new QSocketNotifier(w.fd, QSocketNotifier::Read, qApp);
        QObject::connect(notifier, &QSocketNotifier::activated, qApp,
                         [stats, line = w.line, fd = w.fd]() {
            // Nur einsammeln, nicht ausgeben: eine prellende Leitung wuerde
            // sonst das Terminal zumuellen und die Ausgabe selbst wuerde den
            // Takt bestimmen. Den Bericht macht der Zeitgeber unten.
            gpio_v2_line_event ev{};
            int count = 0;
            while (count++ < 64 && ::read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                LineStat& st = (*stats)[line];
                Edge e;
                e.ns = ev.timestamp_ns;
                e.falling = (ev.id == GPIO_V2_LINE_EVENT_FALLING_EDGE);
                if (st.haveLast && e.ns > st.lastNs)
                    e.gapNs = e.ns - st.lastNs;
                st.lastNs = e.ns;
                st.haveLast = true;
                if (st.edges.size() < 4000)
                    st.edges.append(e);
            }
        });
    }

    auto* stormHinted = new bool(false);
    auto* report = new QTimer(qApp);
    report->setInterval(250);
    QObject::connect(report, &QTimer::timeout, qApp, [stats, stormHinted]() {
        for (auto it = stats->begin(); it != stats->end(); ++it) {
            LineStat& st = it.value();
            if (st.edges.isEmpty())
                continue;
            const int line = it.key();
            const QString when = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            const QString who = QString("BCM %1 (Pin %2)").arg(line, 3).arg(headerPin(line), 2);

            if (st.edges.size() <= 2) {
                // So sieht ein sauberer Tastendruck aus: einzeln auflisten.
                for (const Edge& e : st.edges) {
                    out() << when << "  " << who << "  "
                          << (e.falling ? "fallend (gedrueckt)" : "steigend (losgelassen)");
                    if (e.gapNs > 0)
                        out() << "   " << nsText(e.gapNs) << " nach der vorigen Flanke";
                    out() << "\n";
                }
            } else {
                int falling = 0;
                quint64 minGap = 0;
                for (const Edge& e : st.edges) {
                    if (e.falling)
                        falling++;
                    if (e.gapNs > 0 && (minGap == 0 || e.gapNs < minGap))
                        minGap = e.gapNs;
                }
                const quint64 span = st.edges.last().ns - st.edges.first().ns;
                out() << when << "  " << who << "  " << st.edges.size() << " Flanken in "
                      << nsText(span) << " (" << falling << " fallend, "
                      << (st.edges.size() - falling) << " steigend), kuerzester Abstand "
                      << nsText(minGap) << "  -> prellt\n";
                if (!*stormHinted) {
                    *stormHinted = true;
                    out() << "\n"
                             "Hinweis: So schnell aufeinander folgende Flanken sind Prellen oder\n"
                             "Einstreuung, kein Tastendruck. Die Fotobox selbst entprellt, der\n"
                             "Kernel verwirft dabei alles innerhalb von gpioDebounceMs (Standard\n"
                             "30 ms). Ruhiger wird die Leitung durch einen externen Pull-up\n"
                             "(10 kOhm nach 3,3 V), 100 nF vom Pin nach GND und ein kurzes Kabel,\n"
                             "das nicht parallel zu Netz- oder Blitzleitungen laeuft.\n\n";
                }
            }
            st.edges.clear();
        }
        out().flush();
    });
    report->start();

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
