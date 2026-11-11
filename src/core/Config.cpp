#include "Config.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

static QString expandHome(QString p)
{
    if (p == "~")
        return QDir::homePath();
    if (p.startsWith("~/"))
        return QDir::homePath() + p.mid(1);
    return p;
}

Config Config::defaults()
{
    Config c;
    c.basePath  = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/Fotobox";
    c.yearPath  = QString::number(QDate::currentDate().year());
    c.partyPath = "Test_Event";
    // Absichtlich leer: der Name steht auf der QR-Karte, dort soll ohne eigene
    // Angabe lieber nichts stehen als ein Platzhalter.
    c.partyname.clear();
    c.link      = "https://example.com";
    return c;
}

QString Config::findDefaultPath()
{
    QStringList candidates;

    const QString env = qEnvironmentVariable("FOTOBOX_CONFIG");
    if (!env.isEmpty())
        candidates << env;

    candidates << QDir::currentPath() + "/config.json";
    candidates << QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + "/fotobox/config.json";

    for (const QString& c : candidates)
        if (QFile::exists(c))
            return c;

    return {};
}

QString Config::defaultSavePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/fotobox/config.json";
}

QString Config::systemPath()
{
    const QString env = qEnvironmentVariable("FOTOBOX_SYSTEM_CONFIG");
    return env.isEmpty() ? QStringLiteral("/etc/fotobox/config.json") : env;
}

QString Config::eventConfigPath() const
{
    if (eventConfigFile.isEmpty() || basePath.isEmpty())
        return {};
    return QDir::cleanPath(expandHome(basePath) + "/" + eventConfigFile);
}

// Liest eine Datei ueber die bestehenden Werte. Felder, die fehlen, bleiben
// unveraendert. allowPaths=false ignoriert basePath: die Nextcloud-Datei liegt
// selbst darin und koennte sich sonst den Boden unter den Fuessen wegziehen.
bool Config::applyFile(const QString& path, bool allowPaths)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    const QByteArray roh = f.readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(roh, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        // Zeilennummer aus der Position im Text, das hilft beim Suchen.
        const int zeile = roh.left(err.offset).count('\n') + 1;
        const QString grund = err.error != QJsonParseError::NoError
                                  ? QString("%1 (Zeile %2)").arg(err.errorString()).arg(zeile)
                                  : QStringLiteral("kein JSON-Objekt");
        qWarning() << "Konfiguration ungueltig, wird uebersprungen:" << path << grund;
        problems << QString("%1: %2").arg(QFileInfo(path).fileName(), grund);
        return false;
    }

    Config& c = *this;
    const QJsonObject root = doc.object();
    const QJsonObject sys = root.value("SystemData").toObject();
    if (!allowPaths && sys.contains("basePath"))
        qWarning() << "basePath in" << path << "wird ignoriert: die Datei liegt selbst darin.";
    if (allowPaths)
        c.basePath = sys.value("basePath").toString(c.basePath);
    c.yearPath  = sys.value("yearPath").toString(c.yearPath);
    c.partyPath = sys.value("partyPath").toString(c.partyPath);
    // Beim Tippen am Handy rutscht leicht ein Leerzeichen an den Rand.
    c.partyname = sys.value("partyname").toString(c.partyname).trimmed();
    c.link      = sys.value("link").toString(c.link).trimmed();
    c.password  = sys.value("password").toString(c.password).trimmed();
    c.eventConfigFile = sys.value("eventConfigFile").toString(c.eventConfigFile);

    const QJsonObject beh = root.value("Behaviour").toObject();
    c.countdownSeconds    = beh.value("countdownSeconds").toInt(c.countdownSeconds);
    c.slideshowIntervalMs = beh.value("slideshowIntervalMs").toInt(c.slideshowIntervalMs);
    c.photoDisplayMs      = beh.value("photoDisplayMs").toInt(c.photoDisplayMs);
    c.liveViewTimeoutMs   = beh.value("liveViewTimeoutMs").toInt(c.liveViewTimeoutMs);
    c.slideshowEnabled    = beh.value("slideshowEnabled").toBool(c.slideshowEnabled);
    c.showQr              = beh.value("showQr").toBool(c.showQr);
    c.qrSize              = beh.value("qrSize").toInt(c.qrSize);
    c.theme               = beh.value("theme").toString(c.theme);

    const QJsonObject hw = root.value("Hardware").toObject();
    c.cameraBackend   = hw.value("camera").toString(c.cameraBackend);
    c.liveViewBackend = hw.value("liveView").toString(c.liveViewBackend);
    c.triggerBackend  = hw.value("trigger").toString(c.triggerBackend);
    c.autoTriggerMs   = hw.value("autoTriggerMs").toInt(c.autoTriggerMs);
    c.gphoto2Binary   = hw.value("gphoto2Binary").toString(c.gphoto2Binary);
    c.rpicamBinary    = hw.value("rpicamBinary").toString(c.rpicamBinary);
    c.liveViewDevice  = hw.value("liveViewDevice").toString(c.liveViewDevice);
    c.liveViewWidth   = hw.value("liveViewWidth").toInt(c.liveViewWidth);
    c.liveViewHeight  = hw.value("liveViewHeight").toInt(c.liveViewHeight);
    c.liveViewFps     = hw.value("liveViewFps").toInt(c.liveViewFps);
    c.gpioChip        = hw.value("gpioChip").toString(c.gpioChip);
    c.gpioLine        = hw.value("gpioLine").toInt(c.gpioLine);
    c.gpioPull        = hw.value("gpioPull").toString(c.gpioPull);
    c.gpioDebounceMs  = hw.value("gpioDebounceMs").toInt(c.gpioDebounceMs);
    c.ledBackend      = hw.value("led").toString(c.ledBackend);
    c.ledBrightness   = hw.value("ledBrightness").toInt(c.ledBrightness);
    c.ledPwmChip      = hw.value("ledPwmChip").toInt(c.ledPwmChip);
    c.ledPwmChannel   = hw.value("ledPwmChannel").toInt(c.ledPwmChannel);
    c.ledPwmFrequency = hw.value("ledPwmFrequency").toInt(c.ledPwmFrequency);

    c.loadedFrom << path;
    return true;
}

Config Config::load(const QString& path)
{
    Config c = defaults();
    if (path.isEmpty())
        return c;
    if (!c.applyFile(path, true))
        qWarning() << "Konfiguration nicht lesbar:" << path;
    else
        c.sourcePath = path;
    return c;
}

Config Config::loadLayered(const QString& explicitPath)
{
    Config c = defaults();
    c.problems.clear();

    // 1. systemweit
    c.applyFile(systemPath(), true);

    // 2. persoenlich: hierhin schreibt "Speichern"
    const QString userPath = defaultSavePath();
    c.applyFile(userPath, true);
    c.sourcePath = userPath;

    // 3. ausdruecklich genannte Datei
    QString explicitly = explicitPath;
    if (explicitly.isEmpty())
        explicitly = qEnvironmentVariable("FOTOBOX_CONFIG");
    c.explicitPath = explicitly;
    if (!explicitly.isEmpty()) {
        if (c.applyFile(explicitly, true))
            c.sourcePath = explicitly;
        else
            qWarning() << "Konfiguration nicht lesbar:" << explicitly;
    }

    // 4. Nextcloud: Link, Passwort und Eventname, vom Handy aus aenderbar
    const QString eventPath = c.eventConfigPath();
    if (!eventPath.isEmpty())
        c.applyFile(eventPath, false);

    if (c.loadedFrom.isEmpty())
        qWarning() << "Keine Konfiguration gefunden, nutze Standardwerte."
                   << "Erwartet z.B. unter" << userPath;
    else
        qInfo().noquote() << "Konfiguration aus:" << c.loadedFrom.join(" + ");

    return c;
}

bool Config::save()
{
    // Ohne geladene Datei an den Standardort schreiben, nicht ins gerade
    // aktuelle Arbeitsverzeichnis: das ist je nach Start ein anderes.
    if (sourcePath.isEmpty())
        sourcePath = defaultSavePath();

    const QString dir = QFileInfo(sourcePath).absolutePath();
    if (!QDir().mkpath(dir)) {
        qWarning() << "Ordner nicht anzulegen:" << dir;
        return false;
    }

    // Vorhandene Datei einlesen, damit nichts verloren geht.
    QJsonObject root;
    QFile in(sourcePath);
    if (in.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(in.readAll());
        if (doc.isObject())
            root = doc.object();
        in.close();
    }

    // Geschrieben wird ausschliesslich, was sich am Geraet einstellen laesst.
    //
    // Frueher landete der gesamte wirksame Stand hier, also auch Werte aus der
    // Nextcloud-Datei wie partyPath oder link. Die froren damit in der
    // persoenlichen Ebene ein: wurden sie spaeter in der Nextcloud-Datei
    // entfernt, galt weiter die alte Kopie. Was dort nicht hingehoert, wird
    // auch nicht angefasst; von Hand Eingetragenes bleibt erhalten.
    QJsonObject beh = root.value("Behaviour").toObject();
    beh["theme"]             = theme;
    beh["countdownSeconds"]  = countdownSeconds;
    beh["photoDisplayMs"]    = photoDisplayMs;
    beh["liveViewTimeoutMs"] = liveViewTimeoutMs;
    beh["slideshowEnabled"]  = slideshowEnabled;
    beh["showQr"]            = showQr;
    root["Behaviour"] = beh;

    QJsonObject hw = root.value("Hardware").toObject();
    hw["ledBrightness"]  = ledBrightness;
    hw["liveViewDevice"] = liveViewDevice;
    root["Hardware"] = hw;

    QFile out(sourcePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Konfiguration nicht schreibbar:" << sourcePath;
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    qInfo() << "Konfiguration gespeichert:" << sourcePath;
    return true;
}

QJsonObject Config::toJson() const
{
    QJsonObject sys{
        {"basePath", basePath}, {"yearPath", yearPath}, {"partyPath", partyPath},
        {"partyname", partyname}, {"link", link},
        {"password", password.isEmpty() ? QString() : QStringLiteral("(gesetzt)")},
        {"eventConfigFile", eventConfigFile},
    };
    QJsonObject beh{
        {"countdownSeconds", countdownSeconds}, {"slideshowIntervalMs", slideshowIntervalMs},
        {"photoDisplayMs", photoDisplayMs}, {"liveViewTimeoutMs", liveViewTimeoutMs},
        {"slideshowEnabled", slideshowEnabled},
        {"showQr", showQr}, {"qrSize", qrSize}, {"theme", theme},
    };
    QJsonObject hw{
        {"camera", cameraBackend}, {"liveView", liveViewBackend}, {"trigger", triggerBackend},
        {"led", ledBackend}, {"gphoto2Binary", gphoto2Binary}, {"rpicamBinary", rpicamBinary},
        {"liveViewDevice", liveViewDevice},
        {"liveViewWidth", liveViewWidth}, {"liveViewHeight", liveViewHeight},
        {"liveViewFps", liveViewFps},
        {"gpioChip", gpioChip}, {"gpioLine", gpioLine}, {"gpioPull", gpioPull},
        {"gpioDebounceMs", gpioDebounceMs},
        {"ledBrightness", ledBrightness}, {"ledPwmChip", ledPwmChip},
        {"ledPwmChannel", ledPwmChannel}, {"ledPwmFrequency", ledPwmFrequency},
    };
    return QJsonObject{{"SystemData", sys}, {"Behaviour", beh}, {"Hardware", hw}};
}

QString Config::fullImagePath() const
{
    return QDir::cleanPath(expandHome(basePath) + "/" + yearPath + "/" + partyPath);
}
