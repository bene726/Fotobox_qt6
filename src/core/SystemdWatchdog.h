#pragma once
#include <QByteArray>
#include <QObject>
#include <QTimer>

// Lebenszeichen an systemd ("WATCHDOG=1", wie sd_notify es sendet).
//
// Der Ping kommt aus einem Timer im Qt-Eventloop, und genau darin liegt der
// Zweck: blockiert der Eventloop, bleibt das Lebenszeichen aus und systemd
// startet den Dienst neu. Ein Einfrieren der Oberflaeche ist kein Absturz -
// der Prozess lebt, antwortet aber nicht mehr - und faellt dem Dienst ohne
// Watchdog nicht auf.
//
// Verlangt "WatchdogSec=" in der Unit; systemd nennt das Fenster dann in
// WATCHDOG_USEC. Ohne systemd oder ohne diesen Eintrag bleibt die Klasse
// still und untaetig, damit die Anwendung auch von Hand startbar bleibt.
class SystemdWatchdog : public QObject
{
    Q_OBJECT
public:
    explicit SystemdWatchdog(QObject* parent = nullptr);
    ~SystemdWatchdog() override;

    bool isActive() const { return m_fd >= 0; }

private:
    void ping();

    int m_fd = -1;
    QByteArray m_socketPath;   // Pfad aus NOTIFY_SOCKET, "@" meint abstrakt
    bool m_warned = false;     // Fehler nur einmal melden, nicht je Intervall
    QTimer m_timer;
};
