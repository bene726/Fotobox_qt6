#include <QApplication>
#include <QCommandLineParser>

#include "MainWindow.h"
#include "core/Config.h"
#include "ui/Theme.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QPainter>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("fotobox");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Fotobox");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption optConfig({"c", "config"}, "Pfad zur config.json", "datei");
    QCommandLineOption optWindowed({"w", "windowed"}, "Im Fenster statt Vollbild starten");
    QCommandLineOption optCamera("camera", "Kamera-Backend: auto|dummy|gphoto", "backend");
    QCommandLineOption optLiveView("liveview", "Livebild-Backend: auto|dummy|rpicam", "backend");
    QCommandLineOption optTrigger("trigger", "Trigger-Backend: auto|keyboard|gpio|timer", "backend");
    QCommandLineOption optTheme("theme", "Design: retro|neon|elegant|wedding-blush|wedding-green|pool", "id");
    QCommandLineOption optScreenshot("screenshot", "Nach 2 s ein Bild speichern und beenden", "datei");
    QCommandLineOption optShow("show", "Startzustand fuer Screenshots: liveview|settings|countdown", "zustand");
    parser.addOptions({optConfig, optWindowed, optCamera, optLiveView, optTrigger, optTheme,
                       optScreenshot, optShow});
    parser.process(app);

    const QString cfgPath = parser.isSet(optConfig) ? parser.value(optConfig)
                                                    : Config::findDefaultPath();
    Config cfg = Config::load(cfgPath);

    if (parser.isSet(optCamera))   cfg.cameraBackend   = parser.value(optCamera);
    if (parser.isSet(optLiveView)) cfg.liveViewBackend = parser.value(optLiveView);
    if (parser.isSet(optTrigger))  cfg.triggerBackend  = parser.value(optTrigger);
    if (parser.isSet(optTheme))    cfg.theme           = parser.value(optTheme);

    // Optionale Schriften: fonts/ neben dem Programm, im Arbeitsverzeichnis oder in der Config-Nähe.
    Theme::loadFonts({QCoreApplication::applicationDirPath() + "/fonts",
                      QDir::currentPath() + "/fonts",
                      QFileInfo(cfg.sourcePath).absolutePath() + "/fonts"});

    MainWindow w(cfg);
    if (parser.isSet(optWindowed)) {
        w.resize(1024, 600);
        w.show();
    } else {
        w.showFullScreen();
    }

    if (parser.isSet(optShow))
        w.showDemoState(parser.value(optShow));

    if (parser.isSet(optScreenshot)) {
        const QString file = parser.value(optScreenshot);
        QTimer::singleShot(2000, &w, [&w, file] {
            QPixmap shot = w.grab();
            // Offenes Popup (z.B. Dropdown) mit ins Bild malen.
            if (QWidget* popup = QApplication::activePopupWidget()) {
                QPainter p(&shot);
                p.drawPixmap(popup->mapToGlobal(QPoint(0, 0)) - w.mapToGlobal(QPoint(0, 0)),
                             popup->grab());
            }
            if (shot.save(file))
                qInfo() << "Screenshot gespeichert:" << file;
            else
                qWarning() << "Screenshot fehlgeschlagen:" << file;
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
