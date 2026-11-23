// Tests fuer die Ueberwachung des Bildordners.
//
// Der Watcher entscheidet, was in der Diashow landet. Uebersieht er eine
// Endung, fehlt das Foto; meldet er eine halb geschriebene Datei, zeigt die
// Fotobox ein grau abgeschnittenes Bild.

#include <QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "core/FileWatcher.h"

namespace {

// Legt eine Datei mit Inhalt an und setzt ihre Aenderungszeit, damit die
// Sortierung nach Alter pruefbar ist.
//
// Die Zeit wird bewusst erst nach dem Schliessen gesetzt: wird sie vorher
// gesetzt, ueberschreibt das Schreiben der gepufferten Daten beim Schliessen
// sie wieder. Dann tragen alle Dateien praktisch dieselbe Zeit, und die
// Reihenfolge haengt vom Zufall ab.
bool makeFile(const QString& path, const QDateTime& mtime = {})
{
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        f.write("x");
        f.close();
    }
    if (!mtime.isValid())
        return true;

    QFile f(path);   // ohne Truncate: es wird nichts mehr geschrieben
    if (!f.open(QIODevice::ReadWrite))
        return false;
    return f.setFileTime(mtime, QFileDevice::FileModificationTime);
}

} // namespace

class TestFileWatcher : public QObject
{
    Q_OBJECT

private slots:
    void isImageFile_data();
    void isImageFile();

    void imageFilesListsOnlyImages();
    void imageFilesSortsOldestFirst();
    void imageFilesOnMissingDirIsEmpty();
    void imageFilesReturnsAbsolutePaths();

    void constructorCreatesDirectory();
    void existingFilesAreNotReported();
    void newImageIsReported();
    void newNonImageIsIgnored();
    void sameFileIsReportedOnlyOnce();
};

void TestFileWatcher::isImageFile_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("expected");

    QTest::newRow("jpg")              << "foto.jpg"            << true;
    QTest::newRow("JPG gross")        << "FOTO.JPG"            << true;
    QTest::newRow("jpeg")             << "foto.jpeg"           << true;
    QTest::newRow("JPEG gemischt")    << "foto.JpEg"           << true;
    QTest::newRow("png")              << "foto.png"            << true;
    QTest::newRow("PNG gemischt")     << "foto.PnG"            << true;
    QTest::newRow("mit pfad")         << "/srv/bilder/a.jpg"    << true;
    QTest::newRow("punkt im ordner")  << "/srv/v1.2/a.png"      << true;
    QTest::newRow("mehrere punkte")   << "foto.2026.jpg"       << true;

    QTest::newRow("text")             << "notiz.txt"           << false;
    QTest::newRow("roh vom sensor")   << "IMG_0001.cr2"        << false;
    QTest::newRow("nikon roh")        << "DSC_0001.nef"        << false;
    QTest::newRow("ohne endung")      << "foto"                << false;
    QTest::newRow("leer")             << ""                    << false;
    QTest::newRow("nur punkt")        << "."                   << false;
    QTest::newRow("endung getarnt")   << "foto.jpg.txt"        << false;
    QTest::newRow("archiv")           << "bilder.tar.gz"       << false;
    QTest::newRow("halb geschrieben") << "foto.jpg.part"       << false;
    QTest::newRow("versteckt")        << ".foto.jpg"           << true;
}

void TestFileWatcher::isImageFile()
{
    QFETCH(QString, name);
    QFETCH(bool, expected);

    QCOMPARE(FileWatcher::isImageFile(name), expected);
}

void TestFileWatcher::imageFilesListsOnlyImages()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    QVERIFY(makeFile(dir.filePath("a.jpg")));
    QVERIFY(makeFile(dir.filePath("b.PNG")));
    QVERIFY(makeFile(dir.filePath("c.jpeg")));
    QVERIFY(makeFile(dir.filePath("notiz.txt")));
    QVERIFY(makeFile(dir.filePath("IMG_1.cr2")));
    QVERIFY(QDir(dir.path()).mkdir("unterordner"));

    QStringList names;
    for (const QString& p : FileWatcher::imageFiles(dir.path()))
        names << QFileInfo(p).fileName();
    names.sort();

    QCOMPARE(names, QStringList({"a.jpg", "b.PNG", "c.jpeg"}));
}

void TestFileWatcher::imageFilesSortsOldestFirst()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    const QDateTime now = QDateTime::currentDateTime();
    // Absichtlich in verdrehter Reihenfolge angelegt: sortiert wird nach Zeit,
    // nicht nach Name oder Anlegereihenfolge.
    QVERIFY(makeFile(dir.filePath("mittags.jpg"), now.addSecs(-3600)));
    QVERIFY(makeFile(dir.filePath("abends.jpg"), now.addSecs(-60)));
    QVERIFY(makeFile(dir.filePath("morgens.jpg"), now.addSecs(-7200)));

    // Erst nachsehen, ob die Zeiten ueberhaupt angekommen sind: sonst
    // meldet ein Fehlschlag die falsche Ursache.
    for (const auto& [name, erwartet] : QList<QPair<QString, QDateTime>>{
             {"morgens.jpg", now.addSecs(-7200)},
             {"mittags.jpg", now.addSecs(-3600)},
             {"abends.jpg", now.addSecs(-60)}}) {
        const QDateTime ist = QFileInfo(dir.filePath(name)).lastModified();
        QVERIFY2(qAbs(ist.secsTo(erwartet)) <= 2,
                 qPrintable(QString("%1: Zeit nicht gesetzt (%2 statt %3)")
                                .arg(name, ist.toString(), erwartet.toString())));
    }

    QStringList names;
    for (const QString& p : FileWatcher::imageFiles(dir.path()))
        names << QFileInfo(p).fileName();

    // Die Diashow laeuft chronologisch: das erste Foto des Abends zuerst.
    QCOMPARE(names, QStringList({"morgens.jpg", "mittags.jpg", "abends.jpg"}));
}

void TestFileWatcher::imageFilesOnMissingDirIsEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Ein nicht gemounteter USB-Stick oder ein fehlender Nextcloud-Ordner darf
    // nichts Unerwartetes liefern.
    QVERIFY(FileWatcher::imageFiles(dir.filePath("gibt-es-nicht")).isEmpty());
    QVERIFY(FileWatcher::imageFiles(QString()).isEmpty());
}

void TestFileWatcher::imageFilesReturnsAbsolutePaths()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(makeFile(dir.filePath("a.jpg")));

    const QStringList files = FileWatcher::imageFiles(dir.path());
    QCOMPARE(files.size(), 1);
    QVERIFY2(QDir::isAbsolutePath(files.first()), qPrintable(files.first()));
    QVERIFY(QFile::exists(files.first()));
}

void TestFileWatcher::constructorCreatesDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString neu = dir.filePath("2026/Fest");
    QVERIFY(!QDir(neu).exists());

    FileWatcher w(neu);

    // Der Bildordner wird beim Start angelegt, damit das erste Foto nicht
    // daran scheitert.
    QVERIFY(QDir(neu).exists());
}

void TestFileWatcher::existingFilesAreNotReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(makeFile(dir.filePath("alt.jpg")));

    FileWatcher w(dir.path());
    QSignalSpy spy(&w, &FileWatcher::newFile);

    // Beim Start vorhandene Bilder sind bekannt: sonst loest die Fotobox beim
    // Hochfahren eine Flut von Meldungen aus.
    QVERIFY(!spy.wait(1500));
    QCOMPARE(spy.count(), 0);
}

void TestFileWatcher::newImageIsReported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileWatcher w(dir.path());
    QSignalSpy spy(&w, &FileWatcher::newFile);

    const QString neu = dir.filePath("frisch.jpg");
    QVERIFY(makeFile(neu));

    QVERIFY2(spy.wait(10000), "kein newFile-Signal fuer eine neue Bilddatei");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QFileInfo(neu).absoluteFilePath());
}

void TestFileWatcher::newNonImageIsIgnored()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileWatcher w(dir.path());
    QSignalSpy spy(&w, &FileWatcher::newFile);

    QVERIFY(makeFile(dir.filePath("notiz.txt")));
    QVERIFY(makeFile(dir.filePath("IMG_1.cr2")));

    QVERIFY(!spy.wait(2000));
    QCOMPARE(spy.count(), 0);
}

void TestFileWatcher::sameFileIsReportedOnlyOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileWatcher w(dir.path());
    QSignalSpy spy(&w, &FileWatcher::newFile);

    const QString neu = dir.filePath("frisch.jpg");
    QVERIFY(makeFile(neu));
    QVERIFY(spy.wait(10000));

    // gphoto2 schreibt die Datei und ruehrt sie danach noch an. Das darf das
    // Foto nicht ein zweites Mal in die Diashow bringen.
    QVERIFY(makeFile(neu, QDateTime::currentDateTime()));
    QVERIFY(!spy.wait(2000));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestFileWatcher)
#include "tst_filewatcher.moc"
