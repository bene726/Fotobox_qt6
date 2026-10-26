#include "CameraScan.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#ifdef FOTOBOX_HAS_QTMULTIMEDIA
#include <QCameraDevice>
#include <QMediaDevices>
#endif

namespace {
QTextStream& out()
{
    static QTextStream s(stdout);
    return s;
}

// Fallback ohne Qt Multimedia: die Namen stehen auch im Dateisystem.
void listV4l2()
{
    const QStringList nodes = QDir("/sys/class/video4linux").entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                                      QDir::Name);
    if (nodes.isEmpty()) {
        out() << "Keine Videogeraete unter /dev/video* gefunden.\n";
        return;
    }
    out() << "Videogeraete laut System:\n";
    for (const QString& n : nodes) {
        QFile f("/sys/class/video4linux/" + n + "/name");
        QString name = "?";
        if (f.open(QIODevice::ReadOnly))
            name = QString::fromLocal8Bit(f.readAll()).trimmed();
        out() << "  /dev/" << n << "  " << name << "\n";
    }
}
}

QList<QPair<QString, QString>> CameraScan::devices()
{
    QList<QPair<QString, QString>> out;
#ifdef FOTOBOX_HAS_QTMULTIMEDIA
    for (const QCameraDevice& c : QMediaDevices::videoInputs())
        out.append({QString::fromUtf8(c.id()), c.description()});
#endif
    return out;
}

int CameraScan::run()
{
#ifdef FOTOBOX_HAS_QTMULTIMEDIA
    const QList<QCameraDevice> cams = QMediaDevices::videoInputs();
    if (cams.isEmpty()) {
        out() << "Qt findet keine Kamera.\n\n";
        listV4l2();
        out() << "\nHinweis: Eine Spiegelreflex im PTP-Modus ist keine Videoquelle.\n"
                 "Sie wird ueber gphoto2 angesprochen, nicht ueber das Livebild.\n";
        out().flush();
        return 1;
    }

    out() << "Verfuegbare Kameras fuer das Livebild:\n\n";
    for (const QCameraDevice& c : cams) {
        out() << (c == QMediaDevices::defaultVideoInput() ? " * " : "   ")
              << c.description() << "\n"
              << "     Kennung: " << QString::fromUtf8(c.id()) << "\n";
        const QList<QCameraFormat> formats = c.videoFormats();
        QList<QSize> sizes;
        for (const QCameraFormat& f : formats)
            if (!sizes.contains(f.resolution()))
                sizes << f.resolution();
        if (!sizes.isEmpty()) {
            out() << "     Aufloesungen:";
            int n = 0;
            for (const QSize& s : sizes) {
                if (n++ >= 6) { out() << " ..."; break; }
                out() << " " << s.width() << "x" << s.height();
            }
            out() << "\n";
        }
        out() << "\n";
    }
    out() << "Mit * markiert ist die Standardkamera.\n"
             "Eine andere waehlen: \"liveViewDevice\" in der Konfiguration auf einen\n"
             "Teil des Namens setzen, z.B. \"WEB CAM\".\n";
    out().flush();
    return 0;
#else
    out() << "Diese Version wurde ohne Qt Multimedia gebaut.\n\n";
    listV4l2();
    out().flush();
    return 1;
#endif
}
