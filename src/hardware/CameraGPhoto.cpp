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
    m_proc.start(m_binary, {"--capture-image-and-download",
                            "--filename", m_file,
                            "--force-overwrite"});
}

void CameraGPhoto::fail(const QString& msg)
{
    if (!m_busy)
        return;
    m_busy = false;
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
