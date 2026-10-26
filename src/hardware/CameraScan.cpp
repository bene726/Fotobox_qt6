#include "CameraScan.h"

#include "HardwareFactory.h"
#include "LiveViewCamera.h"
#include "core/Config.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>

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

// Die Namen der Videogeraete stehen auch im Dateisystem. Unter Linux immer
// mit ausgeben: Qt zeigt nicht jeden Knoten, und manche Webcams melden einen
// zweiten Knoten, der gar keine Bilder liefert.
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
             "Eine andere waehlen: im Einstellungsmenue oder ueber \"liveViewDevice\".\n\n";
#ifdef __linux__
    listV4l2();
    out() << "\nOb wirklich Bilder ankommen, zeigt: fotobox --test-camera\n";
#endif
    out().flush();
    return 0;
#else
    out() << "Diese Version wurde ohne Qt Multimedia gebaut.\n\n";
    listV4l2();
    out().flush();
    return 1;
#endif
}

int CameraScan::test(const Config& cfg, int seconds)
{
    LiveViewCamera* cam = createLiveViewCamera(cfg, qApp);
    if (!cam) {
        out() << "Kein Livebild-Backend verfuegbar.\n";
        out().flush();
        return 1;
    }

    int frames = 0;
    QSize size;
    QElapsedTimer clock;
    QObject::connect(cam, &LiveViewCamera::frameReady, qApp, [&](const QImage& img) {
        if (frames == 0) {
            size = img.size();
            out() << "Erstes Bild nach " << clock.elapsed() << " ms, "
                  << size.width() << "x" << size.height() << "\n";
            out().flush();
        }
        frames++;
    });

    out() << "Teste " << seconds << " Sekunden lang ...\n";
    out().flush();
    clock.start();
    cam->start();

    int rc = 0;
    QTimer::singleShot(seconds * 1000, qApp, [&] {
        cam->stop();
        const double fps = frames / double(seconds);
        if (frames == 0) {
            out() << "\nKEIN BILD erhalten.\n"
                     "  - Backend pruefen: \"liveView\" (qt = USB-Webcam, rpicam = Pi-Kamera)\n"
                     "  - Belegt ein anderes Programm die Kamera?\n"
                     "  - Gehoert der Benutzer zur Gruppe 'video'?\n";
            rc = 1;
        } else {
            out() << "\n" << frames << " Bilder in " << seconds << " s ("
                  << QString::number(fps, 'f', 1) << " pro Sekunde). Das Livebild funktioniert.\n";
        }
        out().flush();
        qApp->quit();
    });

    qApp->exec();
    return rc;
}
