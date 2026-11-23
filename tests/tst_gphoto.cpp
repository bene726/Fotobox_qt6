// Tests fuer das Auslesen des Akkustands aus der Ausgabe von gphoto2.
//
// Die Ausgabe kommt von einem fremden Programm und haengt am Kameramodell.
// Nicht jeder Body meldet Prozent: manche nennen eine Spannung, manche nur
// ein Wort. Wird so etwas als Zahl gelesen, stuende in der Fotobox "Akku: 7 %"
// bei vollem Akku - schlimmer als gar keine Anzeige.

#include <QtTest>

#include "hardware/CameraGPhoto.h"

class TestGPhoto : public QObject
{
    Q_OBJECT

private slots:
    void parseBatteryLevel_data();
    void parseBatteryLevel();
};

void TestGPhoto::parseBatteryLevel_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<int>("percent");

    // So antwortet eine Canon EOS in PTP auf --get-config batterylevel.
    const QString canon = QStringLiteral(
        "Label: Battery Level\n"
        "Readonly: 0\n"
        "Type: TEXT\n"
        "Current: 50%\n"
        "END\n");
    QTest::newRow("canon") << canon << 50;

    QTest::newRow("voll")
        << QStringLiteral("Label: Battery Level\nCurrent: 100%\nEND\n") << 100;
    QTest::newRow("leer")
        << QStringLiteral("Label: Battery Level\nCurrent: 0%\nEND\n") << 0;
    QTest::newRow("ohne prozentzeichen")
        << QStringLiteral("Current: 75\n") << 75;
    QTest::newRow("kleingeschrieben")
        << QStringLiteral("current: 42%\n") << 42;
    QTest::newRow("eingerueckt")
        << QStringLiteral("   Current:   37%   \n") << 37;
    QTest::newRow("zeilenende windows")
        << QStringLiteral("Current: 61%\r\n") << 61;

    // Alles, was kein Prozentwert ist, gilt als unbekannt.
    QTest::newRow("spannung")
        << QStringLiteral("Label: Battery Level\nCurrent: 7.4\nEND\n") << -1;
    QTest::newRow("wort")
        << QStringLiteral("Current: Low\n") << -1;
    QTest::newRow("ueber hundert")
        << QStringLiteral("Current: 150%\n") << -1;
    QTest::newRow("negativ")
        << QStringLiteral("Current: -5%\n") << -1;
    QTest::newRow("keine zeile")
        << QStringLiteral("Label: Battery Level\nType: TEXT\nEND\n") << -1;
    QTest::newRow("leere ausgabe")
        << QString() << -1;
    QTest::newRow("fehlermeldung")
        << QStringLiteral("*** Error ***\nCould not find the requested config\n") << -1;
    QTest::newRow("keine kamera")
        << QStringLiteral("*** Error: No camera found. ***\n") << -1;
}

void TestGPhoto::parseBatteryLevel()
{
    QFETCH(QString, output);
    QFETCH(int, percent);

    QCOMPARE(CameraGPhoto::parseBatteryLevel(output), percent);
}

QTEST_GUILESS_MAIN(TestGPhoto)
#include "tst_gphoto.moc"
