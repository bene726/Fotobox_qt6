#include "CameraGPhoto.h"

#include <QDateTime>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDir>
#include <QFile>

namespace {

#ifdef __linux__
// gvfs greift auf dem Desktop ungefragt auf angeschlossene Kameras zu und
// belegt sie. Das faellt sonst erst beim ersten Foto auf.
bool gvfsGphotoRunning()
{
    const QStringList pids = QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& pid : pids) {
        bool isPid = false;
        pid.toInt(&isPid);
        if (!isPid)
            continue;
        QFile f("/proc/" + pid + "/comm");
        if (!f.open(QIODevice::ReadOnly))
            continue;
        // Es gibt zwei Prozesse mit unterschiedlichen Namen: den Monitor
        // "gvfs-gphoto2-volume-monitor" (in comm auf 15 Zeichen gekuerzt) und
        // den Dienst "gvfsd-gphoto2". Deshalb beide Teile einzeln pruefen.
        const QString comm = QString::fromLocal8Bit(f.readAll()).trimmed();
        if (comm.contains("gvfs") && comm.contains("gphoto"))
            return true;
    }
    return false;
}
#endif

// gphoto2 meldet technisch. Die haeufigen Faelle werden in eine Anweisung
// uebersetzt, die auf dem Display der Fotobox weiterhilft.
QString friendlyError(const QString& raw)
{
    const QString r = raw.toLower();
    // Die Fehlercodes von libgphoto2 sind unabhaengig von der Sprache und
    // deshalb die zuverlaessigste Zuordnung.
    if (r.contains("(-53"))
        return QString::fromUtf8("Kamera ist von einem anderen Programm belegt. "
                                 "Auf dem Pi: gvfs-gphoto2 beenden.");
    if (r.contains("(-105"))
        return QString::fromUtf8("Keine Kamera gefunden. USB-Kabel prüfen und Kamera einschalten.");
    if (r.contains("could not claim") || r.contains("device busy"))
        return QString::fromUtf8("Kamera ist von einem anderen Programm belegt. "
                                 "Auf dem Pi: gvfs-gphoto2 beenden.");
    if (r.contains("could not detect any camera") || r.contains("no camera found"))
        return QString::fromUtf8("Keine Kamera gefunden. USB-Kabel prüfen und Kamera einschalten.");
    if (r.contains("out of focus"))
        return QString::fromUtf8("Autofokus hat nicht ausgelöst. Mehr Licht, "
                                 "oder Objektiv auf manuellen Fokus stellen.");
    if (r.contains("no space") || r.contains("store_full") || r.contains("card full"))
        return QString::fromUtf8("Speicherkarte der Kamera ist voll.");
    if (r.contains("antwortet nicht"))
        return QString::fromUtf8("Kamera antwortet nicht. Kabel prüfen, Kamera aus- und einschalten.");
    if (r.contains("timeout") || r.contains("i/o error"))
        return QString::fromUtf8("Verbindung zur Kamera gestört. Kabel prüfen.");
    return raw;
}
}

CameraGPhoto::CameraGPhoto(const QString& binary, QObject* parent)
    : Camera(parent)
    , m_binary(binary)
{
    // Feste Sprache: sonst meldet gphoto2 je nach System auf Deutsch und die
    // Zuordnung der Meldungen greift nicht mehr.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    env.insert("LANGUAGE", "C");
    m_proc.setProcessEnvironment(env);

    connect(&m_proc, &QProcess::finished, this, &CameraGPhoto::onFinished);
    connect(&m_proc, &QProcess::errorOccurred, this, &CameraGPhoto::onError);

    // Akkuabfrage: eigener Prozess, gleiche feste Sprache.
    m_info.setProcessEnvironment(env);
    connect(&m_info, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(m_info.readAllStandardOutput());
        const int percent = code == 0 ? parseBatteryLevel(out) : -1;
        if (percent < 0)
            qInfo() << "Kamera meldet keinen Ladestand";
        emit batteryLevel(percent);
    });
    connect(&m_info, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit batteryLevel(-1);
    });

    // Notbremse: ohne sie bliebe die Fotobox stehen, wenn gphoto2 nicht antwortet.
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(kTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (!m_busy)
            return;
        qWarning() << "gphoto2 antwortet seit" << kTimeoutMs / 1000 << "s nicht, wird beendet.";
        // Erst melden, dann abbrechen: sonst kommt onFinished mit dem
        // Abbruchcode zuvor und die Meldung waere "Exit-Code 9".
        fail(QString::fromUtf8("Kamera antwortet nicht"));
        m_proc.kill();
    });

#ifdef __linux__
    if (gvfsGphotoRunning()) {
        qWarning().noquote()
            << "ACHTUNG: gvfs belegt die Kamera. Das erste Foto wird fehlschlagen.\n"
               "         Dauerhaft abstellen mit:\n"
               "           systemctl --user mask gvfs-gphoto2-volume-monitor.service\n"
               "           systemctl --user stop gvfs-gphoto2-volume-monitor.service\n"
               "           pkill -f 'gvfsd?-gphoto2'";
    }
#endif
}

CameraGPhoto::~CameraGPhoto()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.kill();
        m_proc.waitForFinished(2000);
    }
}

void CameraGPhoto::capture(const QString& targetDir)
{
    if (m_busy) {
        emit captureFailed("Aufnahme laeuft bereits");
        return;
    }

    QDir().mkpath(targetDir);
    m_file = newFilePath(targetDir, "photo");
    m_busy = true;

    qInfo() << "gphoto2 Aufnahme nach" << m_file;
    m_timeout.start();
    m_proc.start(m_binary, {"--capture-image-and-download",
                            "--filename", m_file,
                            "--force-overwrite"});
}

void CameraGPhoto::fail(const QString& msg)
{
    if (!m_busy)
        return;
    m_busy = false;
    m_timeout.stop();
    qWarning() << "gphoto2:" << msg;          // Originalmeldung ins Log
    emit captureFailed(friendlyError(msg));   // verstaendlicher Text aufs Display
}

void CameraGPhoto::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        fail("gphoto2 nicht gefunden: " + m_binary);
    // Andere Fehler landen in onFinished().
}

void CameraGPhoto::onFinished(int exitCode, QProcess::ExitStatus status)
{
    m_timeout.stop();
    if (status != QProcess::NormalExit || exitCode != 0) {
        QString err = QString::fromUtf8(m_proc.readAllStandardError()).trimmed();
        if (err.isEmpty())
            err = "gphoto2 Exit-Code " + QString::number(exitCode);
        fail(err);
        return;
    }

    if (!QFile::exists(m_file)) {
        fail("gphoto2 hat keine Datei geliefert: " + m_file);
        return;
    }

    m_busy = false;
    emit captured(m_file);
}

void CameraGPhoto::requestBatteryLevel()
{
    // Waehrend einer Aufnahme nicht dazwischenfunken: gphoto2 belegt die
    // Kamera allein, sonst scheitern beide Zugriffe.
    if (m_busy || m_info.state() != QProcess::NotRunning) {
        emit batteryLevel(-1);
        return;
    }

    m_info.start(m_binary, {"--get-config", "batterylevel"});

    // Notbremse wie bei der Aufnahme. Der Abbruch meldet sich ueber finished
    // mit einem Code ungleich null und endet damit bei "unbekannt".
    QTimer::singleShot(kInfoTimeoutMs, this, [this] {
        if (m_info.state() != QProcess::NotRunning) {
            qWarning() << "Akkuabfrage antwortet nicht, wird beendet.";
            m_info.kill();
        }
    });
}

int CameraGPhoto::parseBatteryLevel(const QString& output)
{
    // gphoto2 antwortet mehrzeilig:
    //   Label: Battery Level
    //   Type: TEXT
    //   Current: 50%
    //   END
    //
    // Nicht jeder Body liefert Prozent. Manche melden eine Spannung ("7.4")
    // oder Worte ("Low"); beides ergibt keinen Balken und gilt als unbekannt.
    const QStringList lines = output.split('\n');
    for (const QString& line : lines) {
        const QString t = line.trimmed();
        if (!t.startsWith(QLatin1String("Current:"), Qt::CaseInsensitive))
            continue;

        QString value = t.mid(8).trimmed();
        if (value.endsWith('%'))
            value.chop(1);

        bool ok = false;
        const int percent = value.trimmed().toInt(&ok);
        if (!ok || percent < 0 || percent > 100)
            return -1;
        return percent;
    }
    return -1;
}
