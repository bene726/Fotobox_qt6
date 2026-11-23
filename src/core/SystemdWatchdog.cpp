#include "SystemdWatchdog.h"

#include <QDebug>

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {
// Haeufiger senden als noetig: so uebersteht der Dienst auch ein verzoegertes
// Intervall, ohne dass systemd gleich eingreift.
constexpr int kPingsPerWindow = 3;
constexpr int kMinIntervalMs = 1000;
}

SystemdWatchdog::SystemdWatchdog(QObject* parent)
    : QObject(parent)
{
    const QByteArray path = qgetenv("NOTIFY_SOCKET");
    if (path.isEmpty())
        return;   // nicht von systemd gestartet

    // Setzt systemd nur, wenn WatchdogSec in der Unit steht.
    bool ok = false;
    const qint64 windowMs = qgetenv("WATCHDOG_USEC").toLongLong(&ok) / 1000;
    if (!ok || windowMs <= 0)
        return;

    // WATCHDOG_PID nennt den Prozess, der melden soll. Kindprozesse erben die
    // Variablen, duerfen aber nicht an seiner Stelle antworten.
    const QByteArray pid = qgetenv("WATCHDOG_PID");
    if (!pid.isEmpty() && pid.toLongLong() != ::getpid())
        return;

    sockaddr_un probe{};
    if (static_cast<size_t>(path.size()) >= sizeof(probe.sun_path)) {
        qWarning() << "NOTIFY_SOCKET ist zu lang, kein Watchdog:" << path;
        return;
    }

    m_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        qWarning() << "Socket fuer das Lebenszeichen nicht anzulegen:" << ::strerror(errno);
        return;
    }
    const int flags = ::fcntl(m_fd, F_GETFD, 0);
    if (flags >= 0)
        ::fcntl(m_fd, F_SETFD, flags | FD_CLOEXEC);

    m_socketPath = path;

    const int interval = qMax(kMinIntervalMs, static_cast<int>(windowMs / kPingsPerWindow));
    m_timer.setInterval(interval);
    connect(&m_timer, &QTimer::timeout, this, &SystemdWatchdog::ping);
    m_timer.start();

    // Sofort ein erstes Lebenszeichen. Bei Type=simple laeuft das Fenster schon
    // waehrend des Aufbaus der Oberflaeche, und der Timer kommt erst zum Zug,
    // wenn der Eventloop laeuft.
    ping();

    qInfo() << "systemd-Watchdog aktiv: Fenster" << windowMs << "ms, Lebenszeichen alle"
            << interval << "ms";
}

SystemdWatchdog::~SystemdWatchdog()
{
    if (m_fd >= 0)
        ::close(m_fd);
}

void SystemdWatchdog::ping()
{
    if (m_fd < 0)
        return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, m_socketPath.constData(), m_socketPath.size());
    socklen_t len = sizeof(addr);
    if (addr.sun_path[0] == '@') {
        // Abstrakter Socket (nur Linux): fuehrende Null, und die Laenge zaehlt
        // genau bis zum letzten Zeichen - ein Abschluss-Null gehoert nicht dazu.
        addr.sun_path[0] = '\0';
        len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + m_socketPath.size());
    }

    static const char msg[] = "WATCHDOG=1";
    const ssize_t sent = ::sendto(m_fd, msg, sizeof(msg) - 1, MSG_NOSIGNAL,
                                  reinterpret_cast<sockaddr*>(&addr), len);
    if (sent < 0) {
        if (!m_warned) {
            m_warned = true;
            qWarning() << "Lebenszeichen an systemd nicht absetzbar:" << ::strerror(errno);
        }
    } else {
        m_warned = false;
    }
}
