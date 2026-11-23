// Tests fuer die Designs.
//
// Ein Design ist eine lange Liste von Farben, Schriften und Texten. Ein neues
// entsteht durch Abschreiben des alten, und dabei bleibt leicht ein Feld
// liegen: eine nicht gesetzte QColor ist ungueltig und wird im Stylesheet zu
// schwarz auf schwarz. Deshalb prueft dieser Test jedes Design vollstaendig,
// nicht nur das gerade eingestellte.

#include <QtTest>

#include <QColor>
#include <QTemporaryDir>

#include "ui/Theme.h"

class TestTheme : public QObject
{
    Q_OBJECT

private slots:
    void allReturnsThemes();
    void idsAreUniqueAndNonEmpty();

    void byIdFindsEveryTheme_data();
    void byIdFindsEveryTheme();
    void byIdFallsBackToRetro_data();
    void byIdFallsBackToRetro();

    void everyThemeHasAllColors_data();
    void everyThemeHasAllColors();
    void everyThemeHasAllTexts_data();
    void everyThemeHasAllTexts();
    void everyThemeProducesStylesheets_data();
    void everyThemeProducesStylesheets();

    void fontFamiliesFallBackToLastCandidate();
    void controlFamilyAvoidsScriptFonts();

    void loadFontsIgnoresMissingDirs();
    void loadFontsIgnoresForeignFiles();

private:
    // Fuellt eine Datenzeile je Design, damit ein Fehlschlag das Design nennt.
    static void addAllThemesAsRows();
};

void TestTheme::addAllThemesAsRows()
{
    QTest::addColumn<QString>("id");
    for (const Theme& t : Theme::all())
        QTest::newRow(qPrintable(t.id)) << t.id;
}

void TestTheme::allReturnsThemes()
{
    const QList<Theme> themes = Theme::all();
    QVERIFY(!themes.isEmpty());
    // Die Kommandozeilenhilfe und das Dropdown nennen sechs Designs.
    QCOMPARE(themes.size(), 6);
}

void TestTheme::idsAreUniqueAndNonEmpty()
{
    QStringList ids;
    QStringList names;
    for (const Theme& t : Theme::all()) {
        QVERIFY(!t.id.isEmpty());
        QVERIFY2(!t.name.isEmpty(), qPrintable(t.id));
        // Die Id steht in der Konfiguration: zwei gleiche waeren nicht
        // auseinanderzuhalten.
        QVERIFY2(!ids.contains(t.id), qPrintable(t.id));
        // Der Name steht im Dropdown.
        QVERIFY2(!names.contains(t.name), qPrintable(t.name));
        ids << t.id;
        names << t.name;
    }
}

void TestTheme::byIdFindsEveryTheme_data()
{
    addAllThemesAsRows();
}

void TestTheme::byIdFindsEveryTheme()
{
    QFETCH(QString, id);
    QCOMPARE(Theme::byId(id).id, id);
}

void TestTheme::byIdFallsBackToRetro_data()
{
    QTest::addColumn<QString>("id");

    QTest::newRow("leer")            << "";
    QTest::newRow("unbekannt")       << "gibtsnicht";
    QTest::newRow("falsche schreibung") << "Retro";
    QTest::newRow("tippfehler")      << "retr";
    QTest::newRow("mit leerzeichen") << " retro";
}

void TestTheme::byIdFallsBackToRetro()
{
    QFETCH(QString, id);

    // Ein Tippfehler in der Konfiguration darf die Fotobox nicht ohne Design
    // starten lassen.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Unbekanntes Theme"));
    QCOMPARE(Theme::byId(id).id, QStringLiteral("retro"));
}

void TestTheme::everyThemeHasAllColors_data()
{
    addAllThemesAsRows();
}

void TestTheme::everyThemeHasAllColors()
{
    QFETCH(QString, id);
    const Theme t = Theme::byId(id);

    struct Named { const char* name; QColor color; };
    const QList<Named> colors{
        {"bg", t.bg}, {"overlay", t.overlay},
        {"panel", t.panel}, {"panelBorder", t.panelBorder},
        {"text", t.text}, {"textMuted", t.textMuted},
        {"accent", t.accent}, {"accentText", t.accentText},
        {"countdownFill", t.countdownFill}, {"countdownRing", t.countdownRing},
        {"countdownText", t.countdownText},
        {"sliderTrack", t.sliderTrack}, {"sliderFill", t.sliderFill},
        {"sliderHandle", t.sliderHandle}, {"sliderHandleBorder", t.sliderHandleBorder},
        {"primaryBg", t.primaryBg}, {"primaryText", t.primaryText},
        {"dangerBg", t.dangerBg}, {"dangerText", t.dangerText},
        {"secondaryBg", t.secondaryBg}, {"secondaryText", t.secondaryText},
        {"secondaryBorder", t.secondaryBorder},
        {"passwordBg", t.passwordBg}, {"passwordBorder", t.passwordBorder},
        {"passwordText", t.passwordText},
    };

    for (const Named& c : colors)
        QVERIFY2(c.color.isValid(),
                 qPrintable(QString("%1: Farbe %2 ist nicht gesetzt").arg(id, c.name)));

    // Der Schatten gilt nur fuer Designs, die ihn nutzen.
    if (t.hardShadow)
        QVERIFY2(t.shadowColor.isValid(), qPrintable(id + ": shadowColor fehlt"));

    // Masse muessen sinnvoll sein, sonst verschwinden Raender und Ringe.
    QVERIFY(t.radius >= 0);
    QVERIFY(t.panelBorderWidth >= 0);
    QVERIFY(t.countdownRingWidth > 0);
    QVERIFY(t.sliderHandleBorderWidth >= 0);
    QVERIFY(t.passwordBorderWidth >= 0);

    // Lesbarkeit: Text und Hintergrund duerfen nicht derselbe Ton sein.
    QVERIFY2(t.text.rgb() != t.panel.rgb(), qPrintable(id + ": Text gleich Panelfarbe"));
    QVERIFY2(t.primaryText.rgb() != t.primaryBg.rgb(),
             qPrintable(id + ": Knopftext gleich Knopffarbe"));
    QVERIFY2(t.countdownText.rgb() != t.countdownFill.rgb(),
             qPrintable(id + ": Countdown-Zahl gleich Fuellfarbe"));
    // Die Akzentfarbe traegt die Warnung in der Statuszeile. Faellt sie mit
    // dem Panel zusammen, bleibt der Hinweis auf den vollen Speicher unsichtbar.
    QVERIFY2(t.accent.rgb() != t.panel.rgb(), qPrintable(id + ": Akzent gleich Panelfarbe"));
}

void TestTheme::everyThemeHasAllTexts_data()
{
    addAllThemesAsRows();
}

void TestTheme::everyThemeHasAllTexts()
{
    QFETCH(QString, id);
    const Theme t = Theme::byId(id);

    struct Named { const char* name; QString text; };
    const QList<Named> texts{
        {"hintText", t.hintText}, {"shutterText", t.shutterText},
        {"slideshowTag", t.slideshowTag}, {"slideshowButton", t.slideshowButton},
        {"countdownCaption", t.countdownCaption}, {"qrTitle", t.qrTitle},
    };

    for (const Named& x : texts)
        QVERIFY2(!x.text.trimmed().isEmpty(),
                 qPrintable(QString("%1: Text %2 fehlt").arg(id, x.name)));

    QVERIFY2(!t.displayFamilies.isEmpty(), qPrintable(id + ": keine Display-Schrift"));
    QVERIFY2(!t.bodyFamilies.isEmpty(), qPrintable(id + ": keine Textschrift"));
    for (const QString& f : t.displayFamilies + t.bodyFamilies)
        QVERIFY2(!f.trimmed().isEmpty(), qPrintable(id + ": leerer Schriftname"));
}

void TestTheme::everyThemeProducesStylesheets_data()
{
    addAllThemesAsRows();
}

void TestTheme::everyThemeProducesStylesheets()
{
    QFETCH(QString, id);
    const Theme t = Theme::byId(id);

    struct Named { const char* name; QString css; };
    const QList<Named> sheets{
        {"centralStyle", t.centralStyle()},
        {"gearStyle", t.gearStyle()},
        {"hintStyle", t.hintStyle()},
        {"shutterStyle", t.shutterStyle()},
        {"tagStyle", t.tagStyle()},
        {"countdownOverlayStyle", t.countdownOverlayStyle()},
        {"countdownCircleStyle", t.countdownCircleStyle(200)},
        {"countdownLabelStyle", t.countdownLabelStyle()},
        {"countdownCaptionStyle", t.countdownCaptionStyle()},
        {"messageStyle", t.messageStyle()},
        {"qrCardStyle", t.qrCardStyle()},
        {"settingsStyle", t.settingsStyle()},
        {"comboPopupStyle", t.comboPopupStyle()},
        {"shadowStyle", t.shadowStyle()},
    };

    for (const Named& s : sheets) {
        const QString wo = QString("%1/%2").arg(id, s.name);
        QVERIFY2(!s.css.isEmpty(), qPrintable(wo + ": leeres Stylesheet"));
        // Ein nicht ersetzter Platzhalter faellt im Stylesheet nicht auf: Qt
        // ueberliest die Regel einfach.
        QVERIFY2(!s.css.contains('%'), qPrintable(wo + ": Platzhalter uebrig: " + s.css));
        // Jede Farbe muss als rgba() dastehen; eine vergessene arg()-Ersetzung
        // hinterlaesst sonst eine Regel, die Qt stillschweigend ueberliest.
        QVERIFY2(s.css.contains("rgba("), qPrintable(wo + ": keine Farbe im Stylesheet"));
    }
}

void TestTheme::fontFamiliesFallBackToLastCandidate()
{
    for (const Theme& t : Theme::all()) {
        // Auf dem Pi ist keine der Wunschschriften installiert; trotzdem muss
        // ein Name herauskommen, sonst steht im Stylesheet font-family: "".
        QVERIFY2(!t.displayFamily().isEmpty(), qPrintable(t.id));
        QVERIFY2(!t.bodyFamily().isEmpty(), qPrintable(t.id));
        QVERIFY2(!t.controlFamily().isEmpty(), qPrintable(t.id));
        QVERIFY2(t.displayFamilies.contains(t.displayFamily()), qPrintable(t.id));
        QVERIFY2(t.bodyFamilies.contains(t.bodyFamily()), qPrintable(t.id));
    }
}

void TestTheme::controlFamilyAvoidsScriptFonts()
{
    for (const Theme& t : Theme::all()) {
        // Eine Schreibschrift ist in Zahlen und auf Knoepfen unleserlich,
        // deshalb nutzen die dort die Textschrift.
        if (t.scriptDisplay)
            QCOMPARE(t.controlFamily(), t.bodyFamily());
        else
            QCOMPARE(t.controlFamily(), t.displayFamily());
    }
}

void TestTheme::loadFontsIgnoresMissingDirs()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QCOMPARE(Theme::loadFonts({}), 0);
    QCOMPARE(Theme::loadFonts({dir.filePath("fonts")}), 0);
    QCOMPARE(Theme::loadFonts({QString()}), 0);
    // Ein leerer Ordner ist kein Fehler: Schriften sind optional.
    QCOMPARE(Theme::loadFonts({dir.path()}), 0);
}

void TestTheme::loadFontsIgnoresForeignFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile f(dir.filePath("liesmich.txt"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("keine Schrift");
    f.close();

    QCOMPARE(Theme::loadFonts({dir.path()}), 0);
}

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
