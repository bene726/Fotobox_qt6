// Tests fuer den QR-Code: die eingebundene Bibliothek qrcodegen und den
// Qt-Aufsatz, der daraus ein Bild macht.
//
// qrcodegen ist Fremdcode im Quellbaum (src/ui/qrcodegen.cpp, rund 830 Zeilen).
// Er wird nicht gepflegt, aber er kann versehentlich mitgeaendert werden - beim
// Formatieren, beim Suchen und Ersetzen, beim Aufraeumen von Warnungen. Die
// Tests hier pruefen deshalb Eigenschaften, die in der QR-Norm stehen, nicht
// das, was das Programm gerade tut: Groessen aus den Kapazitaetstabellen,
// Sucher-, Takt- und Formatmuster.
//
// Faellt einer dieser Tests, sind die erzeugten Codes nicht mehr lesbar - und
// das merkt sonst niemand, bis am Abend des Events ein Handy davor steht.

#include <QtTest>

#include <QCryptographicHash>
#include <QImage>
#include <QString>

#include "ui/QrEncoder.h"
#include "ui/qrcodegen.hpp"

using qrcodegen::QrCode;

namespace {

QString repeat(const QString& s, int n)
{
    QString out;
    out.reserve(s.size() * n);
    for (int i = 0; i < n; ++i)
        out += s;
    return out;
}

QrCode encode(const QString& text, QrCode::Ecc ecc = QrCode::Ecc::LOW)
{
    return QrCode::encodeText(text.toUtf8().constData(), ecc);
}

} // namespace

class TestQrEncoder : public QObject
{
    Q_OBJECT

private slots:
    // --- qrcodegen gegen die Norm
    void versionFollowsCapacityTable_data();
    void versionFollowsCapacityTable();
    void sizeMatchesVersion_data();
    void sizeMatchesVersion();
    void finderPatternsAreInPlace();
    void timingPatternsAlternate();
    void formatDarkModuleIsSet();
    void modulesOutsideAreLight();
    void higherEccNeedsMoreSpace();
    void tooMuchDataThrows();
    void moduleMatrixIsUnchanged();

    // --- Qt-Aufsatz
    void encodeScalesEveryModule_data();
    void encodeScalesEveryModule();
    void encodeUsesBlackOnWhite();
    void emptyTextGivesPlaceholder();
    void overlongTextGivesPlaceholder();
    void umlautsAreEncodedAsUtf8();
};

// Die Kapazitaeten stehen in der QR-Norm (ISO/IEC 18004), Tabelle 7:
// Version 1 mit Fehlerkorrektur L traegt 41 Ziffern, 25 alphanumerische
// Zeichen oder 17 Bytes; Version 2 traegt 77 / 47 / 32. Genau an diesen
// Grenzen muss die Bibliothek eine Version weiterschalten.
void TestQrEncoder::versionFollowsCapacityTable_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("version");

    QTest::newRow("41 ziffern -> V1")  << repeat("0123456789", 4) + "0" << 1;
    QTest::newRow("42 ziffern -> V2")  << repeat("0123456789", 4) + "01" << 2;

    QTest::newRow("25 alnum -> V1")    << QString("ABCDEFGHIJKLMNOPQRSTUVWXY") << 1;
    QTest::newRow("26 alnum -> V2")    << QString("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << 2;

    // Kleinbuchstaben gehoeren nicht zum alphanumerischen Satz der Norm,
    // deshalb wird byteweise kodiert.
    QTest::newRow("17 bytes -> V1")    << QString("abcdefghijklmnopq") << 1;
    QTest::newRow("18 bytes -> V2")    << QString("abcdefghijklmnopqr") << 2;
}

void TestQrEncoder::versionFollowsCapacityTable()
{
    QFETCH(QString, text);
    QFETCH(int, version);

    QCOMPARE(encode(text).getVersion(), version);
}

void TestQrEncoder::sizeMatchesVersion_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("kurz")      << QString("abc");
    QTest::newRow("link")      << QString("https://cloud.example.com/s/AbCdEfGh");
    QTest::newRow("lang")      << repeat("fotobox/", 40);
    QTest::newRow("sehr lang") << repeat("0123456789", 100);
}

void TestQrEncoder::sizeMatchesVersion()
{
    QFETCH(QString, text);
    const QrCode qr = encode(text);

    // Norm: Seitenlaenge = 4 * Version + 17.
    QCOMPARE(qr.getSize(), 4 * qr.getVersion() + 17);
    QVERIFY(qr.getVersion() >= 1 && qr.getVersion() <= 40);
}

void TestQrEncoder::finderPatternsAreInPlace()
{
    const QrCode qr = encode("https://cloud.example.com/s/AbCdEfGh");
    const int n = qr.getSize();

    // Jedes der drei Sucherquadrate ist 7x7: aussen ein dunkler Ring, darin
    // ein heller Ring, in der Mitte 3x3 dunkel. Ohne sie findet kein Handy
    // den Code.
    const QList<QPoint> ecken{{0, 0}, {n - 7, 0}, {0, n - 7}};
    for (const QPoint& o : ecken) {
        for (int dy = 0; dy < 7; ++dy) {
            for (int dx = 0; dx < 7; ++dx) {
                const int ring = qMin(qMin(dx, dy), qMin(6 - dx, 6 - dy));
                const bool dunkel = (ring != 1);   // Ring 1 ist der helle
                QVERIFY2(qr.getModule(o.x() + dx, o.y() + dy) == dunkel,
                         qPrintable(QString("Sucher bei %1,%2 falsch an %3,%4")
                                        .arg(o.x()).arg(o.y()).arg(dx).arg(dy)));
            }
        }
    }

    // Die Trennlinie rundherum ist hell.
    for (int i = 0; i < 8; ++i) {
        QVERIFY(!qr.getModule(7, i));
        QVERIFY(!qr.getModule(i, 7));
        QVERIFY(!qr.getModule(n - 8, i));
        QVERIFY(!qr.getModule(i, n - 8));
    }

    // Die vierte Ecke traegt kein Sucherquadrat - daran erkennt ein Leser die
    // Drehung.
    int dunkelInEcke = 0;
    for (int dy = 0; dy < 7; ++dy)
        for (int dx = 0; dx < 7; ++dx)
            if (qr.getModule(n - 7 + dx, n - 7 + dy))
                dunkelInEcke++;
    QVERIFY(dunkelInEcke < 40);   // ein Sucherquadrat haette genau 33 von 49
}

void TestQrEncoder::timingPatternsAlternate()
{
    const QrCode qr = encode("https://cloud.example.com/s/AbCdEfGh");
    const int n = qr.getSize();

    // Zeile und Spalte 6 wechseln dunkel/hell und geben dem Leser das Raster.
    for (int i = 8; i < n - 8; ++i) {
        const bool dunkel = (i % 2 == 0);
        QVERIFY2(qr.getModule(i, 6) == dunkel, qPrintable(QString("Takt waagerecht bei %1").arg(i)));
        QVERIFY2(qr.getModule(6, i) == dunkel, qPrintable(QString("Takt senkrecht bei %1").arg(i)));
    }
}

void TestQrEncoder::formatDarkModuleIsSet()
{
    for (const QString& text : {QStringLiteral("a"), QStringLiteral("0123456789"),
                                QStringLiteral("https://x.test/abc")}) {
        const QrCode qr = encode(text);
        // Die Norm schreibt dieses eine Modul fest als dunkel vor.
        QVERIFY2(qr.getModule(8, qr.getSize() - 8), qPrintable(text));
    }
}

void TestQrEncoder::modulesOutsideAreLight()
{
    const QrCode qr = encode("abc");
    const int n = qr.getSize();

    // Ausserhalb liegt die Ruhezone. getModule darf dort nicht lesen, sondern
    // hell meldet.
    QVERIFY(!qr.getModule(-1, 0));
    QVERIFY(!qr.getModule(0, -1));
    QVERIFY(!qr.getModule(n, 0));
    QVERIFY(!qr.getModule(0, n));
    QVERIFY(!qr.getModule(n, n));
    QVERIFY(!qr.getModule(-1000, -1000));
}

void TestQrEncoder::higherEccNeedsMoreSpace()
{
    const QString text = repeat("fotobox", 10);

    const int low = encode(text, QrCode::Ecc::LOW).getVersion();
    const int medium = encode(text, QrCode::Ecc::MEDIUM).getVersion();
    const int high = encode(text, QrCode::Ecc::HIGH).getVersion();

    QVERIFY2(low <= medium, qPrintable(QString("L=%1 M=%2").arg(low).arg(medium)));
    QVERIFY2(medium <= high, qPrintable(QString("M=%1 H=%2").arg(medium).arg(high)));
    QVERIFY2(low < high, qPrintable(QString("L=%1 H=%2").arg(low).arg(high)));
}

void TestQrEncoder::tooMuchDataThrows()
{
    // Version 40 mit Fehlerkorrektur L traegt 2953 Bytes. Mehr geht nicht, und
    // die Bibliothek sagt das mit einer Ausnahme.
    const QString passt = repeat("a", 2953);
    QVERIFY_THROWS_NO_EXCEPTION(encode(passt));
    QCOMPARE(encode(passt).getVersion(), 40);

    QVERIFY_THROWS_EXCEPTION(qrcodegen::data_too_long, encode(repeat("a", 2954)));
}

void TestQrEncoder::moduleMatrixIsUnchanged()
{
    // Fingerabdruck des fertigen Musters. Er stammt aus dieser Bibliothek und
    // beweist nicht, dass sie richtig rechnet - das pruefen die Tests oben
    // gegen die Norm. Er faellt aber auf, wenn jemand qrcodegen.cpp aus
    // Versehen anfasst, etwa beim Formatieren.
    const QrCode qr = QrCode::encodeText("https://fotobox.example/s/AbCdEfGh",
                                         QrCode::Ecc::LOW);
    QCOMPARE(qr.getVersion(), 3);

    QByteArray bits;
    for (int y = 0; y < qr.getSize(); ++y)
        for (int x = 0; x < qr.getSize(); ++x)
            bits += qr.getModule(x, y) ? '1' : '0';

    const QByteArray digest =
        QCryptographicHash::hash(bits, QCryptographicHash::Sha256).toHex();
    QCOMPARE(digest, QByteArray("6f2031b480edf3964a7c684811f489b10da148b72cb11740d89c61118d144d1f"));
}

void TestQrEncoder::encodeScalesEveryModule_data()
{
    QTest::addColumn<int>("scale");

    QTest::newRow("1")  << 1;
    QTest::newRow("3")  << 3;
    QTest::newRow("8")  << 8;
    QTest::newRow("13") << 13;
}

void TestQrEncoder::encodeScalesEveryModule()
{
    QFETCH(int, scale);

    const QString text = "https://cloud.example.com/s/AbCdEfGh";
    const QrCode qr = encode(text);
    const QImage img = QrEncoderQt::encode(text, scale);

    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), qr.getSize() * scale);
    QCOMPARE(img.height(), qr.getSize() * scale);

    // Jedes Modul muss als geschlossener Block aus scale x scale Pixeln
    // erscheinen; ein Rundungsfehler macht den Code unlesbar.
    for (int y = 0; y < qr.getSize(); ++y) {
        for (int x = 0; x < qr.getSize(); ++x) {
            const QRgb want = qr.getModule(x, y) ? qRgb(0, 0, 0) : qRgb(255, 255, 255);
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    const QRgb got = img.pixel(x * scale + dx, y * scale + dy);
                    if (got != want) {
                        QFAIL(qPrintable(QString("Modul %1,%2 bei Faktor %3 falsch")
                                             .arg(x).arg(y).arg(scale)));
                    }
                }
            }
        }
    }
}

void TestQrEncoder::encodeUsesBlackOnWhite()
{
    const QImage img = QrEncoderQt::encode("https://x.test/abc", 1);
    QVERIFY(!img.isNull());

    // Die Ecke oben links gehoert zum Sucherquadrat und ist immer dunkel.
    QCOMPARE(img.pixel(0, 0), qRgb(0, 0, 0));
    // Genau zwei Farben, sonst verwaschen Skalierung oder Druck den Code.
    QSet<QRgb> farben;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            farben.insert(img.pixel(x, y));
    QCOMPARE(farben, QSet<QRgb>({qRgb(0, 0, 0), qRgb(255, 255, 255)}));
}

void TestQrEncoder::emptyTextGivesPlaceholder()
{
    // Ohne Link soll ein Platzhalter stehen, keine Ausnahme und kein leeres Bild.
    const QImage img = QrEncoderQt::encode(QString(), 8);
    QVERIFY(!img.isNull());
    QCOMPARE(img.size(), QSize(200, 200));
}

void TestQrEncoder::overlongTextGivesPlaceholder()
{
    // Der Link kommt aus der Nextcloud-Datei und wird vom Handy aus getippt.
    // Passt er nicht in einen QR-Code, darf die Fotobox nicht abstuerzen.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("QR-Code"));
    const QImage img = QrEncoderQt::encode(repeat("a", 5000), 4);
    QVERIFY(!img.isNull());
    QCOMPARE(img.size(), QSize(200, 200));
}

void TestQrEncoder::umlautsAreEncodedAsUtf8()
{
    // Der Eventname kann im Link stehen. Mehrbyte-Zeichen brauchen mehr Platz,
    // duerfen aber nicht abgeschnitten werden.
    const QString mit = QString::fromUtf8("https://x.test/Schützenfest");
    const QString ohne = QString::fromUtf8("https://x.test/Schutzenfest");

    const QrCode a = encode(mit);
    const QrCode b = encode(ohne);
    QVERIFY(a.getVersion() >= b.getVersion());
    QVERIFY(!QrEncoderQt::encode(mit, 2).isNull());
}

QTEST_MAIN(TestQrEncoder)
#include "tst_qrencoder.moc"
