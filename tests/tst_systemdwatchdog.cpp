// Tests fuer das Lebenszeichen an systemd.
//
// Der Watchdog ist die Antwort auf das Einfrieren der Oberflaeche: ein
// haengender Prozess lebt noch, antwortet aber nicht mehr, und
// "Restart=on-failure" merkt davon nichts. Entscheidend ist dabei weniger das
// Senden als das Schweigen: laeuft die Fotobox von Hand oder ohne WatchdogSec
// in der Unit, darf die Klasse gar nichts tun.
//
// Getestet wird gegen einen echten Unix-Datagramm-Socket, so wie systemd ihn
// bereitstellt - nur eben im Temporaerordner.

#include <QtTest>

#include <QDir>
#include <QFile>

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "core/SystemdWatchdog.h"

namespace {

// Nimmt die Stelle von systemd ein: bindet einen Datagramm-Socket und sammelt,
// was dort ankommt.
class FakeSystemd
{
public:
    explicit FakeSystemd(const QString& path)
        : m_path(path)
    {
        m_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (m_fd < 0)
            return;
        ::unlink(qPrintable(m_path));

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        const QByteArray raw = m_path.toUtf8();
        std::memcpy(addr.sun_path, raw.constData(),
                    qMin(raw.size(), static_cast<qsizetype>(sizeof(addr.sun_path) - 1)));
        if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    ~FakeSystemd()
    {
        if (m_fd >= 0)
            ::close(m_fd);
        ::unlink(qPrintable(m_path));
    }

    FakeSystemd(const FakeSystemd&) = delete;
    FakeSystemd& operator=(const FakeSystemd&) = delete;

    bool isValid() const { return m_fd >= 0; }
    QString path() const { return m_path; }

    // Liest alles, was anliegt, ohne zu warten.
    QList<QByteArray> drain()
    {
        QList<QByteArray> out;
        char buf[256];
        for (;;) {
            const ssize_t n = ::recv(m_fd, buf, sizeof(buf), MSG_DONTWAIT);
            if (n <= 0)
                break;
            out << QByteArray(buf, static_cast<int>(n));
        }
        return out;
    }

    int count() { return static_cast<int>(drain().size()); }

private:
    QString m_path;
    int m_fd = -1;
};

} // namespace

class TestSystemdWatchdog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void silentWithoutNotifySocket();
    void silentWithoutWindow_data();
    void silentWithoutWindow();
    void silentForAnotherProcess();
    void activeForOwnProcess();
    void silentWhenPathTooLong();
    void silentWhenSocketIsGone();

    void constructorSendsFirstSign();
    void pingsRepeatWithinWindow();
    void intervalHasLowerBound();
    void stopsAfterDestruction();

private:
    // Kurzer Pfad: sun_path hat je nach System nur rund 100 Zeichen Platz.
    QString socketPath() const
    {
        return QDir::tempPath() + QString("/fb-wd-%1.sock").arg(::getpid());
    }
};

void TestSystemdWatchdog::init()
{
    qunsetenv("NOTIFY_SOCKET");
    qunsetenv("WATCHDOG_USEC");
    qunsetenv("WATCHDOG_PID");
}

void TestSystemdWatchdog::cleanup()
{
    qunsetenv("NOTIFY_SOCKET");
    qunsetenv("WATCHDOG_USEC");
    qunsetenv("WATCHDOG_PID");
    QFile::remove(socketPath());
}

void TestSystemdWatchdog::silentWithoutNotifySocket()
{
    // Von Hand gestartet: systemd setzt nichts, und die Fotobox soll trotzdem
    // normal laufen.
    qputenv("WATCHDOG_USEC", "30000000");
    const SystemdWatchdog wd;
    QVERIFY(!wd.isActive());
}

void TestSystemdWatchdog::silentWithoutWindow_data()
{
    QTest::addColumn<QByteArray>("usec");

    QTest::newRow("nicht gesetzt") << QByteArray();
    QTest::newRow("leer")          << QByteArray("");
    QTest::newRow("null")          << QByteArray("0");
    QTest::newRow("negativ")       << QByteArray("-1000000");
    QTest::newRow("kein zahl")     << QByteArray("spaeter");
    QTest::newRow("zu klein")      << QByteArray("500");   // 0,5 ms -> gerundet 0
}

void TestSystemdWatchdog::silentWithoutWindow()
{
    QFETCH(QByteArray, usec);

    FakeSystemd sd(socketPath());
    QVERIFY2(sd.isValid(), ::strerror(errno));
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    if (!usec.isNull())
        qputenv("WATCHDOG_USEC", usec);

    // Ohne WatchdogSec in der Unit gibt es kein Fenster, in dem ein
    // Lebenszeichen zaehlen wuerde.
    const SystemdWatchdog wd;
    QVERIFY(!wd.isActive());
    QCOMPARE(sd.count(), 0);
}

void TestSystemdWatchdog::silentForAnotherProcess()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "3000000");
    qputenv("WATCHDOG_PID", QByteArray::number(static_cast<qint64>(::getpid()) + 1));

    // Kindprozesse (gphoto2, rpicam-vid) erben die Variablen, duerfen aber
    // nicht fuer die Fotobox antworten.
    const SystemdWatchdog wd;
    QVERIFY(!wd.isActive());
    QCOMPARE(sd.count(), 0);
}

void TestSystemdWatchdog::activeForOwnProcess()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "3000000");
    qputenv("WATCHDOG_PID", QByteArray::number(static_cast<qint64>(::getpid())));

    const SystemdWatchdog wd;
    QVERIFY(wd.isActive());
}

void TestSystemdWatchdog::silentWhenPathTooLong()
{
    const QByteArray zuLang(static_cast<int>(sizeof(sockaddr_un::sun_path)) + 10, 'x');
    qputenv("NOTIFY_SOCKET", "/tmp/" + zuLang);
    qputenv("WATCHDOG_USEC", "3000000");

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("NOTIFY_SOCKET ist zu lang"));
    const SystemdWatchdog wd;
    QVERIFY(!wd.isActive());
}

void TestSystemdWatchdog::silentWhenSocketIsGone()
{
    // Der Pfad existiert nicht: der Socket laesst sich anlegen, das Senden
    // scheitert. Gemeldet wird das einmal, und die Anwendung laeuft weiter.
    qputenv("NOTIFY_SOCKET", (QDir::tempPath() + "/fb-wd-gibt-es-nicht.sock").toUtf8());
    qputenv("WATCHDOG_USEC", "3000000");

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("nicht absetzbar"));
    const SystemdWatchdog wd;
    QVERIFY(wd.isActive());   // bereit, nur kommt nichts an
}

void TestSystemdWatchdog::constructorSendsFirstSign()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "30000000");

    const SystemdWatchdog wd;
    QVERIFY(wd.isActive());

    // Bei Type=simple laeuft das Fenster schon, waehrend die Oberflaeche
    // aufgebaut wird - der erste Ping darf nicht auf den Timer warten.
    const QList<QByteArray> msgs = sd.drain();
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs.first(), QByteArray("WATCHDOG=1"));
}

void TestSystemdWatchdog::pingsRepeatWithinWindow()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "3000000");   // 3 s Fenster -> 1 s Abstand

    const SystemdWatchdog wd;
    QVERIFY(wd.isActive());
    QCOMPARE(sd.count(), 1);               // der aus dem Konstruktor

    // In einem Fenster muessen mehrere Lebenszeichen Platz haben, sonst
    // greift systemd schon bei einem einzigen verzoegerten Durchlauf ein.
    QTest::qWait(2500);
    const int n = sd.count();
    QVERIFY2(n >= 2, qPrintable(QString("nur %1 Lebenszeichen in 2,5 s").arg(n)));
}

void TestSystemdWatchdog::intervalHasLowerBound()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "300000");    // 0,3 s Fenster

    const SystemdWatchdog wd;
    QVERIFY(wd.isActive());
    QCOMPARE(sd.count(), 1);

    // Ein Drittel davon waeren 100 ms. So oft soll die Klasse nicht senden:
    // das belastet den Eventloop mehr als es hilft, also gilt eine Untergrenze
    // von einer Sekunde.
    QTest::qWait(700);
    QCOMPARE(sd.count(), 0);
}

void TestSystemdWatchdog::stopsAfterDestruction()
{
    FakeSystemd sd(socketPath());
    QVERIFY(sd.isValid());
    qputenv("NOTIFY_SOCKET", sd.path().toUtf8());
    qputenv("WATCHDOG_USEC", "3000000");

    {
        const SystemdWatchdog wd;
        QVERIFY(wd.isActive());
        QCOMPARE(sd.count(), 1);
    }

    // Nach dem Beenden darf kein Lebenszeichen mehr kommen: beim geordneten
    // Beenden ueber Esc soll systemd den Dienst nicht fuer tot halten, aber
    // auch nicht fuer lebendig.
    QTest::qWait(1500);
    QCOMPARE(sd.count(), 0);
}

QTEST_GUILESS_MAIN(TestSystemdWatchdog)
#include "tst_systemdwatchdog.moc"
