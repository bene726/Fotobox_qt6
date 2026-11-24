// Tests fuer die Uebersetzung der gphoto2-Meldungen.
//
// gphoto2 meldet technisch und auf Englisch. Was davon auf dem Display der
// Fotobox landet, entscheidet diese Zuordnung - und sie ist das einzige, was
// jemand am Abend zu lesen bekommt. Schickt sie zur falschen Ursache, sucht
// man an der falschen Stelle.
//
// Geprueft wird gegen Ausgaben, wie gphoto2 sie wirklich schreibt. Eine
// Kamera braucht es dafuer nicht.

#include <QtTest>

#include "hardware/CameraGPhoto.h"

class TestGPhoto : public QObject
{
    Q_OBJECT

private slots:
    void friendlyError_data();
    void friendlyError();

    void fullStorageDoesNotBlameTheCamera_data();
    void fullStorageDoesNotBlameTheCamera();

    void unknownMessagesArePassedThrough();
};

void TestGPhoto::friendlyError_data()
{
    QTest::addColumn<QString>("raw");
    QTest::addColumn<QString>("enthaelt");

    // Der haeufigste Fall auf dem Pi: der Dateimanager-Dienst greift zuerst zu.
    QTest::newRow("belegt, code")
        << "*** Error (-53: 'Could not claim the USB device') ***"
        << "belegt";
    QTest::newRow("belegt, text")
        << "ERROR: Could not claim the USB device"
        << "belegt";
    QTest::newRow("device busy")
        << "Could not claim the USB device: Device busy"
        << "belegt";

    QTest::newRow("keine kamera, code")
        << "*** Error (-105: 'Unknown model') ***"
        << "Keine Kamera gefunden";
    QTest::newRow("keine kamera, text")
        << "*** Error ***\nCould not detect any camera"
        << "Keine Kamera gefunden";
    QTest::newRow("no camera found")
        << "No camera found."
        << "Keine Kamera gefunden";

    QTest::newRow("autofokus")
        << "*** Error ***\nCanon EOS Capture failed: Out of Focus"
        << "Autofokus";

    QTest::newRow("eigene zeitueberschreitung")
        << "Kamera antwortet nicht"
        << "antwortet nicht";
    QTest::newRow("timeout")
        << "ERROR: Timeout reading from or writing to the port."
        << "Verbindung";
    QTest::newRow("ein-ausgabefehler")
        << "*** Error ***\nAn I/O error occurred"
        << "Verbindung";

    // Gross- und Kleinschreibung darf keine Rolle spielen: die Meldungen
    // kommen je nach Version unterschiedlich.
    QTest::newRow("gross geschrieben")
        << "COULD NOT DETECT ANY CAMERA"
        << "Keine Kamera gefunden";
}

void TestGPhoto::friendlyError()
{
    QFETCH(QString, raw);
    QFETCH(QString, enthaelt);

    const QString text = CameraGPhoto::friendlyError(raw);
    QVERIFY2(text.contains(enthaelt),
             qPrintable(QString("\"%1\" ergab \"%2\", erwartet war \"%3\"")
                            .arg(raw, text, enthaelt)));
    // Die Rohmeldung gehoert ins Protokoll, nicht auf das Display.
    QVERIFY2(text != raw, qPrintable(raw));
}

void TestGPhoto::fullStorageDoesNotBlameTheCamera_data()
{
    QTest::addColumn<QString>("raw");

    QTest::newRow("no space")   << "*** Error ***\nNo space left on device";
    QTest::newRow("store_full") << "*** Error (-1: 'PTP Store Full') ***\nSTORE_FULL";
    QTest::newRow("card full")  << "ERROR: Card full";
}

void TestGPhoto::fullStorageDoesNotBlameTheCamera()
{
    QFETCH(QString, raw);

    // Die Aufnahme wird sofort heruntergeladen und auf der Kamera nicht
    // behalten - dort liegt sie nur bis zum Uebertragen. Geht der Platz aus,
    // ist also der Pi gemeint. Die fruehere Meldung "Speicherkarte der Kamera
    // ist voll" schickte genau zur falschen Karte.
    const QString text = CameraGPhoto::friendlyError(raw);

    QVERIFY2(text.contains(QString::fromUtf8("Speicherplatz")), qPrintable(text));
    QVERIFY2(text.contains("Pi"), qPrintable(text));
    QVERIFY2(!text.contains(QString::fromUtf8("Speicherkarte der Kamera")), qPrintable(text));
}

void TestGPhoto::unknownMessagesArePassedThrough()
{
    // Was die Zuordnung nicht kennt, darf sie nicht verschlucken: lieber die
    // englische Originalmeldung als gar keine.
    const QString fremd = "*** Error ***\nSomething entirely unexpected happened";
    QCOMPARE(CameraGPhoto::friendlyError(fremd), fremd);
    QCOMPARE(CameraGPhoto::friendlyError(QString()), QString());
}

QTEST_GUILESS_MAIN(TestGPhoto)
#include "tst_gphoto.moc"
