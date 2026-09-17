#include "Config.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
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
    c.partyname = "Test_Event";
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

Config Config::load(const QString& path)
{
    Config c = defaults();

    if (path.isEmpty()) {
        qWarning() << "Keine Konfiguration gefunden, nutze Standardwerte."
                   << "Bildordner:" << c.fullImagePath();
        return c;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Konfiguration nicht lesbar:" << path;
        return c;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Konfiguration ungueltig:" << path << err.errorString();
        return c;
    }

    const QJsonObject root = doc.object();

    const QJsonObject sys = root.value("SystemData").toObject();
    c.basePath  = sys.value("basePath").toString(c.basePath);
    c.yearPath  = sys.value("yearPath").toString(c.yearPath);
    c.partyPath = sys.value("partyPath").toString(c.partyPath);
    c.partyname = sys.value("partyname").toString(c.partyname);
    c.link      = sys.value("link").toString(c.link);
    c.password  = sys.value("password").toString(c.password);

    const QJsonObject beh = root.value("Behaviour").toObject();
    c.countdownSeconds    = beh.value("countdownSeconds").toInt(c.countdownSeconds);
    c.slideshowIntervalMs = beh.value("slideshowIntervalMs").toInt(c.slideshowIntervalMs);
    c.photoDisplayMs      = beh.value("photoDisplayMs").toInt(c.photoDisplayMs);
    c.liveViewTimeoutMs   = beh.value("liveViewTimeoutMs").toInt(c.liveViewTimeoutMs);
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
    c.liveViewWidth   = hw.value("liveViewWidth").toInt(c.liveViewWidth);
    c.liveViewHeight  = hw.value("liveViewHeight").toInt(c.liveViewHeight);
    c.liveViewFps     = hw.value("liveViewFps").toInt(c.liveViewFps);
    c.gpioChip        = hw.value("gpioChip").toString(c.gpioChip);
    c.gpioLine        = hw.value("gpioLine").toInt(c.gpioLine);
    c.ledBackend      = hw.value("led").toString(c.ledBackend);
    c.ledBrightness   = hw.value("ledBrightness").toInt(c.ledBrightness);
    c.ledPwmChip      = hw.value("ledPwmChip").toInt(c.ledPwmChip);
    c.ledPwmChannel   = hw.value("ledPwmChannel").toInt(c.ledPwmChannel);

    c.sourcePath = path;
    qInfo() << "Konfiguration geladen:" << path;
    return c;
}

bool Config::save()
{
    if (sourcePath.isEmpty())
        sourcePath = QDir::currentPath() + "/config.json";

    // Vorhandene Datei einlesen, damit nichts verloren geht.
    QJsonObject root;
    QFile in(sourcePath);
    if (in.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(in.readAll());
        if (doc.isObject())
            root = doc.object();
        in.close();
    }

    QJsonObject sys = root.value("SystemData").toObject();
    sys["basePath"]  = basePath;
    sys["yearPath"]  = yearPath;
    sys["partyPath"] = partyPath;
    sys["partyname"] = partyname;
    sys["link"]      = link;
    sys["password"]  = password;
    root["SystemData"] = sys;

    QJsonObject beh = root.value("Behaviour").toObject();
    beh["countdownSeconds"]    = countdownSeconds;
    beh["slideshowIntervalMs"] = slideshowIntervalMs;
    beh["photoDisplayMs"]      = photoDisplayMs;
    beh["liveViewTimeoutMs"]   = liveViewTimeoutMs;
    beh["showQr"]              = showQr;
    beh["qrSize"]              = qrSize;
    beh["theme"]               = theme;
    root["Behaviour"] = beh;

    QJsonObject hw = root.value("Hardware").toObject();
    hw["ledBrightness"] = ledBrightness;
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

QString Config::fullImagePath() const
{
    return QDir::cleanPath(expandHome(basePath) + "/" + yearPath + "/" + partyPath);
}
