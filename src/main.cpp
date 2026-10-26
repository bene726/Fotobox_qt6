#include <QApplication>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QLoggingCategory>

#include "MainWindow.h"
#include "core/Config.h"
#include "hardware/CameraScan.h"
#include "hardware/GpioScan.h"
#include "ui/Theme.h"

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QTextStream>
#include <QTimer>

namespace {

// Alle Kommandozeilenoptionen an einer Stelle.
struct Cli
{
    QCommandLineParser parser;

    QCommandLineOption help = parser.addHelpOption();
    QCommandLineOption version = parser.addVersionOption();

    QCommandLineOption config{{"c", "config"}, "Pfad zur config.json", "datei"};
    QCommandLineOption windowed{{"w", "windowed"}, "Im Fenster statt Vollbild starten"};
    QCommandLineOption camera{"camera", "Kamera-Backend: auto|dummy|snapshot|gphoto", "backend"};
    QCommandLineOption liveView{"liveview", "Livebild-Backend: auto|dummy|qt|rpicam", "backend"};
    QCommandLineOption trigger{"trigger", "Ausloeser: auto|keyboard|gpio|timer", "backend"};
    QCommandLineOption theme{"theme",
                             "Design: retro|neon|elegant|wedding-blush|wedding-green|pool", "id"};
    QCommandLineOption screenshot{"screenshot", "Nach 2 s ein Bild speichern und beenden", "datei"};
    QCommandLineOption show{"show", "Startzustand fuer Screenshots: liveview|settings|countdown|shutdown",
                            "zustand"};
    QCommandLineOption listCameras{"list-cameras", "Verfuegbare Kameras fuer das Livebild auflisten"};
    QCommandLineOption testCamera{"test-camera", "Livebild testen: kommen Bilder an?"};
    QCommandLineOption gpioInfo{"gpio-info", "GPIO-Leitungen auflisten"};
    QCommandLineOption gpioWatch{"gpio-watch", "GPIO-Leitungen beobachten (Pin des Tasters finden)"};

    Cli()
    {
        parser.setApplicationDescription(
            "Fotobox\n\n"
            "Die Optionen --list-cameras, --test-camera, --gpio-info, --gpio-watch, --help\n"
            "und --version\n"
            "brauchen keinen Bildschirm und laufen auch ueber SSH.");
        parser.addOptions({config, windowed, camera, liveView, trigger, theme, screenshot, show,
                           listCameras, testCamera, gpioInfo, gpioWatch});
    }
};

// Die Kamera-Diagnose braucht eine Grafikumgebung, um Bilder umzuwandeln.
// Ueber SSH gibt es keine Anzeige, dann wird offscreen gerendert.
void ensureHeadlessPlatform()
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")
        && qEnvironmentVariableIsEmpty("DISPLAY")
        && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
}

void print(QTextStream& s, const QString& text)
{
    s << text;
    s.flush();
}

// Ohne Anwendungsobjekt kennt Qt den Programmnamen nicht und schreibt
// "<executable_name>" in die Hilfe.
QString helpText(QCommandLineParser& parser)
{
    return parser.helpText().replace("<executable_name>", QCoreApplication::applicationName());
}

} // namespace

int main(int argc, char* argv[])
{
    // Guenstige Webcams haengen an ihre MJPEG-Bilder ein paar Fuellbytes an.
    // Qt meldet das pro Bild, was das Log unbrauchbar macht. Die Bilder sind
    // trotzdem in Ordnung. Mit QT_LOGGING_RULES laesst es sich zurueckholen.
    if (qEnvironmentVariableIsEmpty("QT_LOGGING_RULES"))
        QLoggingCategory::setFilterRules("qt.gui.imageio.jpeg.warning=false");

    QCoreApplication::setApplicationName("fotobox");
#ifdef FOTOBOX_VERSION_STR
    QCoreApplication::setApplicationVersion(FOTOBOX_VERSION_STR);
#else
    QCoreApplication::setApplicationVersion("0.0.0");
#endif

    QStringList args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    // Erst auswerten, dann entscheiden, ob ueberhaupt eine Oberflaeche noetig ist.
    // QCommandLineParser::parse() kommt ohne Anwendungsobjekt aus; damit scheitern
    // Hilfe und Diagnose nicht an einem fehlenden Bildschirm.
    Cli cli;
    QTextStream out(stdout);
    QTextStream err(stderr);

    if (!cli.parser.parse(args)) {
        print(err, cli.parser.errorText() + "\n\n" + helpText(cli.parser));
        return 1;
    }
    if (cli.parser.isSet(cli.help)) {
        print(out, helpText(cli.parser));
        return 0;
    }
    if (cli.parser.isSet(cli.version)) {
        print(out, QCoreApplication::applicationName() + " "
                       + QCoreApplication::applicationVersion() + "\n");
        return 0;
    }

    // Diagnose ohne Oberflaeche
    if (cli.parser.isSet(cli.listCameras)) {
        ensureHeadlessPlatform();
        QGuiApplication app(argc, argv);
        return CameraScan::run();
    }
    if (cli.parser.isSet(cli.testCamera)) {
        ensureHeadlessPlatform();
        QGuiApplication app(argc, argv);
        Config cfg = Config::loadLayered(cli.parser.isSet(cli.config) ? cli.parser.value(cli.config)
                                                                      : QString());
        if (cli.parser.isSet(cli.liveView))
            cfg.liveViewBackend = cli.parser.value(cli.liveView);
        return CameraScan::test(cfg);
    }
    if (cli.parser.isSet(cli.gpioInfo) || cli.parser.isSet(cli.gpioWatch)) {
        QCoreApplication app(argc, argv);
        return GpioScan::run(cli.parser.isSet(cli.gpioWatch));
    }

    QApplication app(argc, argv);

    Config cfg = Config::loadLayered(cli.parser.isSet(cli.config) ? cli.parser.value(cli.config)
                                                                  : QString());

    // Wurde eine Datei ausdruecklich genannt, soll "Speichern" auch dorthin
    // schreiben, selbst wenn sie noch gar nicht existiert.
    if (cli.parser.isSet(cli.config))
        cfg.sourcePath = cli.parser.value(cli.config);

    if (cli.parser.isSet(cli.camera))   cfg.cameraBackend   = cli.parser.value(cli.camera);
    if (cli.parser.isSet(cli.liveView)) cfg.liveViewBackend = cli.parser.value(cli.liveView);
    if (cli.parser.isSet(cli.trigger))  cfg.triggerBackend  = cli.parser.value(cli.trigger);
    if (cli.parser.isSet(cli.theme))    cfg.theme           = cli.parser.value(cli.theme);

    // Optionale Schriften: fonts/ neben dem Programm, im Arbeitsverzeichnis oder in der Config-Nähe.
    Theme::loadFonts({QCoreApplication::applicationDirPath() + "/fonts",
                      QCoreApplication::applicationDirPath() + "/../share/fotobox/fonts",
                      QDir::currentPath() + "/fonts",
                      QFileInfo(cfg.sourcePath).absolutePath() + "/fonts"});

    MainWindow w(cfg);
    if (cli.parser.isSet(cli.windowed)) {
        w.resize(1024, 600);
        w.show();
    } else {
        w.showFullScreen();
    }

    if (cli.parser.isSet(cli.show))
        w.showDemoState(cli.parser.value(cli.show));

    if (cli.parser.isSet(cli.screenshot)) {
        const QString file = cli.parser.value(cli.screenshot);
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
