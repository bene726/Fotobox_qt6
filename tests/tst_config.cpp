// Tests fuer die Konfiguration: Ebenen, Vorrang, kaputte Dateien, Speichern.
//
// Die Konfiguration ist der Teil der Fotobox, der am Abend des Events von Hand
// angefasst wird - vom Handy aus in der Nextcloud-Datei, am Geraet ueber die
// Einstellungen. Hier darf nichts stillschweigend verloren gehen.

#include <QtTest>

#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <csignal>
#include <sys/resource.h>
#include <unistd.h>

#include "core/Config.h"

namespace {

// Schreibt JSON-Text in eine Datei und schlaegt den Test fehl, wenn das nicht geht.
bool writeFile(const QString& path, const QByteArray& text)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(text) == text.size();
}

// Macht jeden Schreibvorgang dieses Prozesses scheitern, so wie es eine volle
// Karte tut: das Anlegen der Datei geht noch (Groesse 0), das Schreiben nicht.
//
// Ueber RLIMIT_FSIZE statt ueber einen schreibgeschuetzten Ordner, denn der
// traefe nur open(). Geprueft werden soll aber der Fall danach - der, bei dem
// frueher niemand hinsah.
class FullDisk
{
public:
    FullDisk()
    {
        // Ohne das beendet das Signal den Testlauf, statt write() scheitern
        // zu lassen.
        m_oldHandler = ::signal(SIGXFSZ, SIG_IGN);
        if (::getrlimit(RLIMIT_FSIZE, &m_old) != 0)
            return;
        rlimit neu = m_old;
        neu.rlim_cur = 0;
        m_active = ::setrlimit(RLIMIT_FSIZE, &neu) == 0;
    }

    ~FullDisk()
    {
        if (m_active)
            ::setrlimit(RLIMIT_FSIZE, &m_old);
        if (m_oldHandler != SIG_ERR)
            ::signal(SIGXFSZ, m_oldHandler);
    }

    FullDisk(const FullDisk&) = delete;
    FullDisk& operator=(const FullDisk&) = delete;

    bool active() const { return m_active; }

private:
    rlimit m_old{};
    bool m_active = false;
    void (*m_oldHandler)(int) = SIG_ERR;
};

QJsonObject readJson(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

} // namespace

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void defaultsAreUsable();

    void loadAppliesValues();
    void loadKeepsDefaultsForMissingKeys();
    void loadRejectsBrokenJson_data();
    void loadRejectsBrokenJson();
    void brokenFileLeavesValuesUntouched();
    void stringsAreTrimmed();

    void eventLayerIgnoresBasePath();

    void fullImagePathJoins_data();
    void fullImagePathJoins();
    void eventConfigPathJoins();
    void eventConfigPathIsEmptyWhenSwitchedOff_data();
    void eventConfigPathIsEmptyWhenSwitchedOff();

    void layersApplyInOrder();
    void layeredWithoutAnyFileUsesDefaults();

    void saveWritesOnlyDeviceSettings();
    void saveKeepsForeignKeys();
    void saveCreatesMissingDirectory();
    void saveRoundTrip();
    void saveReportsFailure();
    void failedSaveKeepsPreviousFile();
    void saveLeavesNoLeftovers();

    void toJsonMasksPassword();

private:
    QString systemConfig() const { return m_tmp.filePath("etc/config.json"); }
    QString userConfig() const { return Config::defaultSavePath(); }

    QTemporaryDir m_tmp;
};

void TestConfig::initTestCase()
{
    QVERIFY2(m_tmp.isValid(), qPrintable(m_tmp.errorString()));
    // Lenkt ConfigLocation und PicturesLocation nach ~/.qttest um, damit die
    // Tests nicht in der echten Konfiguration des Benutzers herumschreiben.
    QStandardPaths::setTestModeEnabled(true);
}

void TestConfig::init()
{
    // Jeder Test beginnt ohne System- und Benutzerebene; wer sie braucht, legt
    // sie selbst an.
    qputenv("FOTOBOX_SYSTEM_CONFIG", systemConfig().toUtf8());
    qunsetenv("FOTOBOX_CONFIG");
    QFile::remove(systemConfig());
    QFile::remove(userConfig());
}

void TestConfig::cleanup()
{
    QFile::remove(systemConfig());
    QFile::remove(userConfig());
    qunsetenv("FOTOBOX_SYSTEM_CONFIG");
    qunsetenv("FOTOBOX_CONFIG");
}

void TestConfig::defaultsAreUsable()
{
    const Config c = Config::defaults();

    QCOMPARE(c.theme, QStringLiteral("retro"));
    QCOMPARE(c.countdownSeconds, 3);
    QCOMPARE(c.cameraBackend, QStringLiteral("auto"));
    QCOMPARE(c.liveViewBackend, QStringLiteral("auto"));
    QCOMPARE(c.triggerBackend, QStringLiteral("auto"));
    QCOMPARE(c.gpioLine, 17);
    QCOMPARE(c.eventConfigFile, QStringLiteral("fotobox.json"));
    QVERIFY(c.slideshowEnabled);
    QVERIFY(c.showQr);

    // Das Jahr kommt aus der Uhr und muss zum Kalender passen, sonst landen
    // die Bilder im falschen Ordner.
    QCOMPARE(c.yearPath, QString::number(QDate::currentDate().year()));

    QVERIFY(!c.basePath.isEmpty());
    QVERIFY(!c.partyPath.isEmpty());
    // Der Name steht auf der QR-Karte: ohne eigene Angabe lieber nichts.
    QVERIFY(c.partyname.isEmpty());

    QVERIFY(c.loadedFrom.isEmpty());
    QVERIFY(c.problems.isEmpty());
}

void TestConfig::loadAppliesValues()
{
    const QString path = m_tmp.filePath("voll.json");
    const QByteArray json = R"({
      "SystemData": {
        "basePath": "/tmp/bilder", "yearPath": "1999", "partyPath": "Fest",
        "partyname": "Unser Fest", "link": "https://beispiel.test/f",
        "password": "geheim", "eventConfigFile": "event.json"
      },
      "Behaviour": {
        "countdownSeconds": 5, "slideshowIntervalMs": 1234, "photoDisplayMs": 2345,
        "liveViewTimeoutMs": 60000, "slideshowEnabled": false, "showQr": false,
        "qrSize": 200, "theme": "neon"
      },
      "Hardware": {
        "camera": "dummy", "liveView": "rpicam", "trigger": "gpio", "led": "pwm",
        "autoTriggerMs": 7000, "gphoto2Binary": "/usr/bin/gphoto2",
        "rpicamBinary": "/usr/bin/rpicam-vid", "liveViewDevice": "C270",
        "liveViewWidth": 640, "liveViewHeight": 480, "liveViewFps": 15,
        "gpioChip": "gpiochip4", "gpioLine": 27, "gpioPull": "down",
        "gpioDebounceMs": 50, "ledBrightness": 80, "ledPwmChip": 1,
        "ledPwmChannel": 2, "ledPwmFrequency": 2000
      }
    })";
    QVERIFY(writeFile(path, json));

    const Config c = Config::load(path);

    QCOMPARE(c.basePath, QStringLiteral("/tmp/bilder"));
    QCOMPARE(c.yearPath, QStringLiteral("1999"));
    QCOMPARE(c.partyPath, QStringLiteral("Fest"));
    QCOMPARE(c.partyname, QStringLiteral("Unser Fest"));
    QCOMPARE(c.link, QStringLiteral("https://beispiel.test/f"));
    QCOMPARE(c.password, QStringLiteral("geheim"));
    QCOMPARE(c.eventConfigFile, QStringLiteral("event.json"));

    QCOMPARE(c.countdownSeconds, 5);
    QCOMPARE(c.slideshowIntervalMs, 1234);
    QCOMPARE(c.photoDisplayMs, 2345);
    QCOMPARE(c.liveViewTimeoutMs, 60000);
    QCOMPARE(c.slideshowEnabled, false);
    QCOMPARE(c.showQr, false);
    QCOMPARE(c.qrSize, 200);
    QCOMPARE(c.theme, QStringLiteral("neon"));

    QCOMPARE(c.cameraBackend, QStringLiteral("dummy"));
    QCOMPARE(c.liveViewBackend, QStringLiteral("rpicam"));
    QCOMPARE(c.triggerBackend, QStringLiteral("gpio"));
    QCOMPARE(c.ledBackend, QStringLiteral("pwm"));
    QCOMPARE(c.autoTriggerMs, 7000);
    QCOMPARE(c.gphoto2Binary, QStringLiteral("/usr/bin/gphoto2"));
    QCOMPARE(c.rpicamBinary, QStringLiteral("/usr/bin/rpicam-vid"));
    QCOMPARE(c.liveViewDevice, QStringLiteral("C270"));
    QCOMPARE(c.liveViewWidth, 640);
    QCOMPARE(c.liveViewHeight, 480);
    QCOMPARE(c.liveViewFps, 15);
    QCOMPARE(c.gpioChip, QStringLiteral("gpiochip4"));
    QCOMPARE(c.gpioLine, 27);
    QCOMPARE(c.gpioPull, QStringLiteral("down"));
    QCOMPARE(c.gpioDebounceMs, 50);
    QCOMPARE(c.ledBrightness, 80);
    QCOMPARE(c.ledPwmChip, 1);
    QCOMPARE(c.ledPwmChannel, 2);
    QCOMPARE(c.ledPwmFrequency, 2000);

    // Geladene Datei wird vermerkt und ist das Ziel von "Speichern".
    QCOMPARE(c.loadedFrom, QStringList{path});
    QCOMPARE(c.sourcePath, path);
    QVERIFY(c.problems.isEmpty());
}

void TestConfig::loadKeepsDefaultsForMissingKeys()
{
    const QString path = m_tmp.filePath("duenn.json");
    const QByteArray json = R"({"Behaviour": {"theme": "pool"}})";
    QVERIFY(writeFile(path, json));

    const Config c = Config::load(path);
    const Config d = Config::defaults();

    QCOMPARE(c.theme, QStringLiteral("pool"));
    // Alles andere bleibt, wie es war. Eine Datei mit einem einzigen Wert darf
    // nicht den Rest der Fotobox zuruecksetzen.
    QCOMPARE(c.countdownSeconds, d.countdownSeconds);
    QCOMPARE(c.basePath, d.basePath);
    QCOMPARE(c.yearPath, d.yearPath);
    QCOMPARE(c.gpioLine, d.gpioLine);
    QCOMPARE(c.showQr, d.showQr);
}

void TestConfig::loadRejectsBrokenJson_data()
{
    QTest::addColumn<QByteArray>("text");

    QTest::newRow("abgeschnitten")  << QByteArray(R"({"Behaviour": {"theme": "neon")");
    QTest::newRow("komma zuviel")   << QByteArray("{\n  \"Behaviour\": {\n    \"theme\": \"neon\",,\n  }\n}");
    QTest::newRow("kein objekt")    << QByteArray("[1, 2, 3]");
    QTest::newRow("leer")           << QByteArray("");
    QTest::newRow("nur text")       << QByteArray("theme = neon");
}

void TestConfig::loadRejectsBrokenJson()
{
    QFETCH(QByteArray, text);

    const QString path = m_tmp.filePath("kaputt.json");
    QVERIFY(writeFile(path, text));

    Config c = Config::defaults();
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Konfiguration ungueltig"));
    QCOMPARE(c.applyFile(path, true), false);

    // Der Grund wird gemeldet, damit ein Tippfehler nicht unbemerkt bleibt.
    QCOMPARE(c.problems.size(), 1);
    QVERIFY2(c.problems.first().contains("kaputt.json"),
             qPrintable(c.problems.first()));
    QVERIFY(c.loadedFrom.isEmpty());
}

void TestConfig::brokenFileLeavesValuesUntouched()
{
    const QString gut = m_tmp.filePath("gut.json");
    const QString schlecht = m_tmp.filePath("schlecht.json");
    const QByteArray gutJson = R"({"Behaviour": {"theme": "elegant", "countdownSeconds": 9}})";
    const QByteArray schlechtJson = R"({"Behaviour": {"theme": "neon")";
    QVERIFY(writeFile(gut, gutJson));
    QVERIFY(writeFile(schlecht, schlechtJson));

    Config c = Config::defaults();
    QVERIFY(c.applyFile(gut, true));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Konfiguration ungueltig"));
    QVERIFY(!c.applyFile(schlecht, true));

    // Eine halb geschriebene Datei darf den bisherigen Stand nicht zerstoeren.
    QCOMPARE(c.theme, QStringLiteral("elegant"));
    QCOMPARE(c.countdownSeconds, 9);
    QCOMPARE(c.loadedFrom, QStringList{gut});
}

void TestConfig::stringsAreTrimmed()
{
    const QString path = m_tmp.filePath("leerzeichen.json");
    const QByteArray json = R"({"SystemData": {
      "partyname": "  Hochzeit  ", "link": " https://a.test/x\n", "password": "\t pw "
    }})";
    QVERIFY(writeFile(path, json));

    const Config c = Config::load(path);

    // Beim Tippen am Handy rutscht leicht ein Leerzeichen an den Rand; im Link
    // waere das fatal, weil der QR-Code dann ins Nichts zeigt.
    QCOMPARE(c.partyname, QStringLiteral("Hochzeit"));
    QCOMPARE(c.link, QStringLiteral("https://a.test/x"));
    QCOMPARE(c.password, QStringLiteral("pw"));
}

void TestConfig::eventLayerIgnoresBasePath()
{
    const QString path = m_tmp.filePath("event.json");
    const QByteArray json = R"({"SystemData": {
      "basePath": "/ganz/woanders", "partyname": "Fest", "link": "https://e.test"
    }})";
    QVERIFY(writeFile(path, json));

    Config c = Config::defaults();
    c.basePath = "/bleibt/so";

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("basePath.*wird ignoriert"));
    QVERIFY(c.applyFile(path, /*allowPaths=*/false));

    // Die Nextcloud-Datei liegt selbst unter basePath und koennte sich sonst
    // den Boden unter den Fuessen wegziehen.
    QCOMPARE(c.basePath, QStringLiteral("/bleibt/so"));
    // Alles andere aus dieser Ebene gilt trotzdem.
    QCOMPARE(c.partyname, QStringLiteral("Fest"));
    QCOMPARE(c.link, QStringLiteral("https://e.test"));
}

void TestConfig::fullImagePathJoins_data()
{
    QTest::addColumn<QString>("basePath");
    QTest::addColumn<QString>("yearPath");
    QTest::addColumn<QString>("partyPath");
    QTest::addColumn<QString>("expected");

    QTest::newRow("einfach")
        << "/srv/bilder" << "2026" << "Fest" << "/srv/bilder/2026/Fest";
    QTest::newRow("schraegstrich am ende")
        << "/srv/bilder/" << "2026" << "Fest" << "/srv/bilder/2026/Fest";
    QTest::newRow("doppelter schraegstrich")
        << "/srv//bilder" << "2026" << "Fest" << "/srv/bilder/2026/Fest";
    QTest::newRow("punkt im pfad")
        << "/srv/./bilder" << "2026" << "Fest" << "/srv/bilder/2026/Fest";
    QTest::newRow("tilde")
        << "~/Bilder" << "2026" << "Fest" << QDir::homePath() + "/Bilder/2026/Fest";
    QTest::newRow("nur tilde")
        << "~" << "2026" << "Fest" << QDir::homePath() + "/2026/Fest";
    QTest::newRow("tilde mitten drin bleibt stehen")
        << "/srv/~/bilder" << "2026" << "Fest" << "/srv/~/bilder/2026/Fest";
    QTest::newRow("leeres jahr")
        << "/srv/bilder" << "" << "Fest" << "/srv/bilder/Fest";
}

void TestConfig::fullImagePathJoins()
{
    QFETCH(QString, basePath);
    QFETCH(QString, yearPath);
    QFETCH(QString, partyPath);
    QFETCH(QString, expected);

    Config c;
    c.basePath = basePath;
    c.yearPath = yearPath;
    c.partyPath = partyPath;

    QCOMPARE(c.fullImagePath(), expected);
}

void TestConfig::eventConfigPathJoins()
{
    Config c;
    c.basePath = "/srv/bilder/";
    c.eventConfigFile = "fotobox.json";
    QCOMPARE(c.eventConfigPath(), QStringLiteral("/srv/bilder/fotobox.json"));

    c.basePath = "~/Nextcloud";
    QCOMPARE(c.eventConfigPath(), QDir::homePath() + "/Nextcloud/fotobox.json");
}

void TestConfig::eventConfigPathIsEmptyWhenSwitchedOff_data()
{
    QTest::addColumn<QString>("basePath");
    QTest::addColumn<QString>("eventConfigFile");

    QTest::newRow("ohne dateiname") << "/srv/bilder" << "";
    QTest::newRow("ohne basis")     << "" << "fotobox.json";
    QTest::newRow("ohne beides")    << "" << "";
}

void TestConfig::eventConfigPathIsEmptyWhenSwitchedOff()
{
    QFETCH(QString, basePath);
    QFETCH(QString, eventConfigFile);

    Config c;
    c.basePath = basePath;
    c.eventConfigFile = eventConfigFile;

    QVERIFY(c.eventConfigPath().isEmpty());
}

void TestConfig::layersApplyInOrder()
{
    const QString base = m_tmp.filePath("nextcloud");
    QVERIFY(QDir().mkpath(base));

    // 1. systemweit
    const QByteArray systemJson = R"({
      "SystemData": {"basePath": "/system/bilder", "partyPath": "System"},
      "Behaviour": {"theme": "neon", "countdownSeconds": 9, "qrSize": 111}
    })";
    QVERIFY(writeFile(systemConfig(), systemJson));

    // 2. persoenlich
    const QByteArray userJson = R"({"Behaviour": {"theme": "elegant"}})";
    QVERIFY(writeFile(userConfig(), userJson));

    // 3. ausdruecklich genannt
    const QString explicitPath = m_tmp.filePath("meine.json");
    const QString explicitTemplate = R"({
      "SystemData": {"basePath": "%1", "partyPath": "Explizit"},
      "Behaviour": {"countdownSeconds": 7}
    })";
    QVERIFY(writeFile(explicitPath, explicitTemplate.arg(base).toUtf8()));

    // 4. Nextcloud, vom Handy aus aenderbar
    const QByteArray eventJson = R"({
      "SystemData": {"basePath": "/darf/nicht/gelten", "partyname": "Fest",
                     "link": "https://cloud.test/s/abc"}
    })";
    QVERIFY(writeFile(base + "/fotobox.json", eventJson));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("basePath.*wird ignoriert"));
    const Config c = Config::loadLayered(explicitPath);

    // Spaetere Ebenen gewinnen ...
    QCOMPARE(c.countdownSeconds, 7);          // explizit schlaegt System
    QCOMPARE(c.theme, QStringLiteral("elegant"));  // persoenlich schlaegt System
    QCOMPARE(c.partyPath, QStringLiteral("Explizit"));
    QCOMPARE(c.basePath, base);               // Nextcloud-basePath ignoriert
    // ... und was keine spaetere Ebene nennt, bleibt aus der frueheren.
    QCOMPARE(c.qrSize, 111);
    // Die Nextcloud-Ebene liefert Eventname und Link.
    QCOMPARE(c.partyname, QStringLiteral("Fest"));
    QCOMPARE(c.link, QStringLiteral("https://cloud.test/s/abc"));

    // Reihenfolge der gelesenen Dateien, so wie --dump-config sie zeigt.
    const QStringList expected{systemConfig(), userConfig(), explicitPath,
                               base + "/fotobox.json"};
    QCOMPARE(c.loadedFrom, expected);
    // "Speichern" schreibt in die ausdruecklich genannte Datei.
    QCOMPARE(c.sourcePath, explicitPath);
    QCOMPARE(c.explicitPath, explicitPath);
}

void TestConfig::layeredWithoutAnyFileUsesDefaults()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Keine Konfiguration gefunden"));
    const Config c = Config::loadLayered();
    const Config d = Config::defaults();

    QVERIFY(c.loadedFrom.isEmpty());
    QCOMPARE(c.theme, d.theme);
    QCOMPARE(c.countdownSeconds, d.countdownSeconds);
    // Ohne Datei zeigt "Speichern" auf den Standardort, nicht ins
    // Arbeitsverzeichnis: das ist je nach Start ein anderes.
    QCOMPARE(c.sourcePath, Config::defaultSavePath());
}

void TestConfig::saveWritesOnlyDeviceSettings()
{
    Config c = Config::defaults();
    c.sourcePath = m_tmp.filePath("speichern.json");
    c.theme = "pool";
    c.countdownSeconds = 6;
    c.ledBrightness = 42;
    c.liveViewDevice = "HD Pro";
    // Werte aus der Nextcloud-Ebene: die gehoeren nicht in die persoenliche Datei.
    c.partyPath = "Hochzeit";
    c.link = "https://cloud.test/s/xyz";
    c.password = "geheim";
    c.partyname = "Anna und Ben";

    QVERIFY(c.save());

    const QJsonObject root = readJson(c.sourcePath);
    const QJsonObject beh = root.value("Behaviour").toObject();
    const QJsonObject hw = root.value("Hardware").toObject();

    QCOMPARE(beh.value("theme").toString(), QStringLiteral("pool"));
    QCOMPARE(beh.value("countdownSeconds").toInt(), 6);
    QCOMPARE(hw.value("ledBrightness").toInt(), 42);
    QCOMPARE(hw.value("liveViewDevice").toString(), QStringLiteral("HD Pro"));

    // Frueher landete der gesamte wirksame Stand hier. Die Werte aus der
    // Nextcloud-Datei froren damit ein: wurden sie dort spaeter entfernt, galt
    // weiter die alte Kopie.
    QVERIFY2(!root.contains("SystemData"), "SystemData gehoert nicht in die Geraetedatei");
    const QByteArray raw = QJsonDocument(root).toJson(QJsonDocument::Compact);
    QVERIFY2(!raw.contains("Hochzeit"), raw.constData());
    QVERIFY2(!raw.contains("cloud.test"), raw.constData());
    QVERIFY2(!raw.contains("geheim"), raw.constData());
    QVERIFY2(!raw.contains("Anna und Ben"), raw.constData());

    // Nur was sich am Geraet einstellen laesst, nichts darueber hinaus.
    const QStringList behKeys = beh.keys();
    QCOMPARE(behKeys, QStringList({"countdownSeconds", "liveViewTimeoutMs", "photoDisplayMs",
                                   "showQr", "slideshowEnabled", "theme"}));
    QCOMPARE(hw.keys(), QStringList({"ledBrightness", "liveViewDevice"}));
}

void TestConfig::saveKeepsForeignKeys()
{
    const QString path = m_tmp.filePath("fremd.json");
    const QByteArray json = R"({
      "SystemData": {"partyPath": "Von Hand", "link": "https://hand.test"},
      "Behaviour": {"theme": "neon", "meinEigenerSchalter": true},
      "Notizen": {"warum": "weil"}
    })";
    QVERIFY(writeFile(path, json));

    Config c = Config::defaults();
    c.sourcePath = path;
    c.theme = "elegant";
    QVERIFY(c.save());

    const QJsonObject root = readJson(path);

    // Von Hand Eingetragenes bleibt erhalten, auch was die Fotobox nicht kennt.
    QCOMPARE(root.value("SystemData").toObject().value("partyPath").toString(),
             QStringLiteral("Von Hand"));
    QCOMPARE(root.value("Behaviour").toObject().value("meinEigenerSchalter").toBool(), true);
    QCOMPARE(root.value("Notizen").toObject().value("warum").toString(), QStringLiteral("weil"));
    // Der eingestellte Wert gewinnt.
    QCOMPARE(root.value("Behaviour").toObject().value("theme").toString(),
             QStringLiteral("elegant"));
}

void TestConfig::saveCreatesMissingDirectory()
{
    Config c = Config::defaults();
    c.sourcePath = m_tmp.filePath("gibt/es/noch/nicht/config.json");

    QVERIFY(c.save());
    QVERIFY(QFile::exists(c.sourcePath));
}

void TestConfig::saveRoundTrip()
{
    Config c = Config::defaults();
    c.sourcePath = m_tmp.filePath("rund.json");
    c.theme = "wedding-green";
    c.countdownSeconds = 8;
    c.photoDisplayMs = 1500;
    c.liveViewTimeoutMs = 42000;
    c.slideshowEnabled = false;
    c.showQr = false;
    c.ledBrightness = 7;
    c.liveViewDevice = "C920";
    QVERIFY(c.save());

    const Config back = Config::load(c.sourcePath);

    QCOMPARE(back.theme, c.theme);
    QCOMPARE(back.countdownSeconds, c.countdownSeconds);
    QCOMPARE(back.photoDisplayMs, c.photoDisplayMs);
    QCOMPARE(back.liveViewTimeoutMs, c.liveViewTimeoutMs);
    QCOMPARE(back.slideshowEnabled, c.slideshowEnabled);
    QCOMPARE(back.showQr, c.showQr);
    QCOMPARE(back.ledBrightness, c.ledBrightness);
    QCOMPARE(back.liveViewDevice, c.liveViewDevice);
}

void TestConfig::saveReportsFailure()
{
    const QString datei = m_tmp.filePath("voll/config.json");
    QVERIFY(QDir().mkpath(QFileInfo(datei).absolutePath()));

    Config c = Config::defaults();
    c.sourcePath = datei;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Konfiguration nicht"));
    bool ok = true;
    {
        const FullDisk voll;
        QVERIFY2(voll.active(), "Schreibgrenze liess sich nicht setzen");
        ok = c.save();
    }

    // Frueher wurde der Rueckgabewert von write() nicht geprueft: save()
    // meldete Erfolg, und auf dem Display stand "Einstellungen gespeichert",
    // obwohl nichts angekommen war.
    QVERIFY2(!ok, "save() meldet Erfolg, obwohl nichts geschrieben wurde");
}

void TestConfig::failedSaveKeepsPreviousFile()
{
    const QString datei = m_tmp.filePath("voll2/config.json");
    QVERIFY(QDir().mkpath(QFileInfo(datei).absolutePath()));
    const QByteArray vorher = R"({"Behaviour": {"theme": "neon", "countdownSeconds": 9}})";
    QVERIFY(writeFile(datei, vorher));

    Config c = Config::defaults();
    c.sourcePath = datei;
    c.theme = "pool";

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Konfiguration nicht"));
    bool ok = true;
    {
        const FullDisk voll;
        QVERIFY(voll.active());
        ok = c.save();
    }
    QVERIFY(!ok);

    // Darum geht es beim atomaren Ersetzen. Frueher leerte Truncate die Datei
    // sofort; schlug das Schreiben danach fehl, blieb nichts uebrig. Jetzt
    // steht weiter die alte Fassung da.
    QFile f(datei);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), vorher);
    f.close();

    const Config zurueck = Config::load(datei);
    QCOMPARE(zurueck.theme, QStringLiteral("neon"));
    QCOMPARE(zurueck.countdownSeconds, 9);
}

void TestConfig::saveLeavesNoLeftovers()
{
    const QString ordner = m_tmp.filePath("sauber");
    QVERIFY(QDir().mkpath(ordner));

    Config c = Config::defaults();
    c.sourcePath = ordner + "/config.json";
    QVERIFY(c.save());
    QVERIFY(c.save());   // zweimal, damit auch ein Ersetzen vorkommt

    // Die Nebendatei von QSaveFile darf nicht liegen bleiben: sonst sammelt
    // sich im Konfigurationsordner mit jedem Speichern eine mehr an.
    QCOMPARE(QDir(ordner).entryList(QDir::Files | QDir::Hidden),
             QStringList{"config.json"});
}

void TestConfig::toJsonMasksPassword()
{
    Config c = Config::defaults();
    c.password = "streng-geheim";

    const QJsonObject sys = c.toJson().value("SystemData").toObject();
    // --dump-config laeuft oft ueber SSH und landet in Logs.
    QCOMPARE(sys.value("password").toString(), QStringLiteral("(gesetzt)"));

    c.password.clear();
    QCOMPARE(c.toJson().value("SystemData").toObject().value("password").toString(), QString());
}

QTEST_MAIN(TestConfig)
#include "tst_config.moc"
