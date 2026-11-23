// Tests fuer die Platzpruefung des Bildordners.
//
// Die Zahl, auf die es ankommt, ist nicht "wie viele Gigabyte", sondern "wie
// viele Fotos noch". Die haengt an der Groesse der Bilder, und die haengt an
// der Kamera - deshalb wird sie aus den schon abgelegten Fotos geschaetzt und
// nicht geraten.

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <unistd.h>

#include "core/Storage.h"

namespace {

bool makeFile(const QString& path, qint64 bytes)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(QByteArray(static_cast<int>(bytes), 'x')) == bytes;
}

} // namespace

class TestStorage : public QObject
{
    Q_OBJECT

private slots:
    void humanBytes_data();
    void humanBytes();

    void levelFollowsThresholds_data();
    void levelFollowsThresholds();

    void unknownWithoutFolder_data();
    void unknownWithoutFolder();

    void emptyFolderUsesEstimate();
    void sizeIsAveragedOverExistingPhotos();
    void nonImagesAreIgnored();
    void photosLeftIsCapped();
    void readOnlyFolderIsReported();
};

void TestStorage::humanBytes_data()
{
    QTest::addColumn<qint64>("bytes");
    QTest::addColumn<QString>("text");

    QTest::newRow("null")        << qint64(0)                      << "0 kB";
    QTest::newRow("ein kB")      << qint64(1024)                   << "1 kB";
    QTest::newRow("anderthalb")  << qint64(1536)                   << "2 kB";
    QTest::newRow("ein MB")      << qint64(1024) * 1024            << "1 MB";
    QTest::newRow("512 MB")      << qint64(1024) * 1024 * 512      << "512 MB";
    QTest::newRow("ein GB")      << qint64(1024) * 1024 * 1024     << "1,0 GB";
    QTest::newRow("3,4 GB")      << qint64(3650722201)             << "3,4 GB";
    QTest::newRow("viel")        << qint64(1024) * 1024 * 1024 * 64 << "64,0 GB";
    QTest::newRow("unsinn")      << qint64(-1)                     << "?";
}

void TestStorage::humanBytes()
{
    QFETCH(qint64, bytes);
    QFETCH(QString, text);

    // Komma statt Punkt: die Oberflaeche ist durchgehend deutsch.
    QCOMPARE(Storage::humanBytes(bytes), text);
}

void TestStorage::levelFollowsThresholds_data()
{
    QTest::addColumn<qint64>("frei");
    QTest::addColumn<int>("level");

    QTest::newRow("reichlich")      << qint64(50) * 1024 * 1024 * 1024
                                    << int(Storage::Level::Ok);
    QTest::newRow("genau 1 GB")     << Storage::kLowBytes
                                    << int(Storage::Level::Ok);
    QTest::newRow("knapp darunter") << Storage::kLowBytes - 1
                                    << int(Storage::Level::Low);
    QTest::newRow("genau 200 MB")   << Storage::kCriticalBytes
                                    << int(Storage::Level::Low);
    QTest::newRow("darunter")       << Storage::kCriticalBytes - 1
                                    << int(Storage::Level::Critical);
    QTest::newRow("leer")           << qint64(0)
                                    << int(Storage::Level::Critical);
}

void TestStorage::levelFollowsThresholds()
{
    QFETCH(qint64, frei);
    QFETCH(int, level);

    QCOMPARE(int(Storage::levelFor(frei)), level);
}

void TestStorage::unknownWithoutFolder_data()
{
    QTest::addColumn<QString>("dir");

    QTest::newRow("leer")        << QString();
    QTest::newRow("gibt es nicht") << QStringLiteral("/gibt/es/ganz/sicher/nicht");
}

void TestStorage::unknownWithoutFolder()
{
    QFETCH(QString, dir);

    // Ohne Ordner liefert QStorageInfo den naechsthoeheren Datentraeger. Das
    // waere eine Zahl, die mit dem Ziel nichts zu tun hat - lieber keine.
    const Storage::Space s = Storage::check(dir);
    QVERIFY(!s.known);
    QCOMPARE(s.freeBytes, qint64(0));
    QCOMPARE(s.photosLeft, 0);
}

void TestStorage::emptyFolderUsesEstimate()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    const Storage::Space s = Storage::check(dir.path());

    QVERIFY(s.known);
    QVERIFY(s.freeBytes > 0);
    // Noch kein Foto da: es gilt die Schaetzung fuer die 600D.
    QCOMPARE(s.bytesPerPhoto, Storage::kDefaultBytesPerPhoto);
    QCOMPARE(s.photosLeft, int(qMin<qint64>(s.freeBytes / s.bytesPerPhoto, Storage::kMaxPhotosLeft)));
    QCOMPARE(s.level, Storage::levelFor(s.freeBytes));
}

void TestStorage::sizeIsAveragedOverExistingPhotos()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(makeFile(dir.filePath("a.jpg"), 1000));
    QVERIFY(makeFile(dir.filePath("b.JPG"), 2000));
    QVERIFY(makeFile(dir.filePath("c.png"), 3000));

    const Storage::Space s = Storage::check(dir.path());

    QVERIFY(s.known);
    // Die Schaetzung soll zur wirklich benutzten Kamera passen, nicht zu der,
    // die im Quelltext steht.
    QCOMPARE(s.bytesPerPhoto, qint64(2000));
    QCOMPARE(s.photosLeft, int(qMin<qint64>(s.freeBytes / 2000, Storage::kMaxPhotosLeft)));
}

void TestStorage::nonImagesAreIgnored()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(makeFile(dir.filePath("a.jpg"), 1000));
    QVERIFY(makeFile(dir.filePath("b.jpg"), 3000));
    // Eine grosse Protokolldatei darf die Schaetzung nicht verziehen.
    QVERIFY(makeFile(dir.filePath("protokoll.txt"), 500000));

    QCOMPARE(Storage::check(dir.path()).bytesPerPhoto, qint64(2000));
}

void TestStorage::photosLeftIsCapped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Winzige "Fotos": rechnerisch passten Millionen davon auf die Platte.
    for (const char* name : {"a.jpg", "b.jpg", "c.jpg"})
        QVERIFY(makeFile(dir.filePath(name), 1));

    const Storage::Space s = Storage::check(dir.path());

    QCOMPARE(s.bytesPerPhoto, qint64(1));
    // Eine fuenfstellige Zahl ist in der Statuszeile schon sinnlos genug.
    QCOMPARE(s.photosLeft, Storage::kMaxPhotosLeft);
}

void TestStorage::readOnlyFolderIsReported()
{
    if (::geteuid() == 0)
        QSKIP("als root sind auch schreibgeschuetzte Ordner beschreibbar");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString ordner = dir.filePath("nur-lesen");
    QVERIFY(QDir().mkpath(ordner));

    QVERIFY(!Storage::check(ordner).readOnly);

    // So verhaelt sich ein nur lesend eingehaengter USB-Stick: der Ordner ist
    // da, aber kein Foto kommt hinein.
    QVERIFY(QFile::setPermissions(ordner, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const Storage::Space s = Storage::check(ordner);
    QFile::setPermissions(ordner, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                      | QFileDevice::ExeOwner);   // damit er loeschbar bleibt

    QVERIFY(s.known);
    QVERIFY(s.readOnly);
}

QTEST_GUILESS_MAIN(TestStorage)
#include "tst_storage.moc"
