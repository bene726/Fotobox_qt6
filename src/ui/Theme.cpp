#include "Theme.h"

#include <QDebug>
#include <QDir>
#include <QFontDatabase>

// ---------------------------------------------------------------- Helfer

static QString rgba(const QColor& c)
{
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

static QString firstInstalled(const QStringList& candidates)
{
    static const QStringList installed = QFontDatabase::families();
    for (const QString& f : candidates)
        if (installed.contains(f))
            return f;
    return candidates.isEmpty() ? QString() : candidates.last();
}

QString Theme::displayFamily() const { return firstInstalled(displayFamilies); }
QString Theme::bodyFamily() const { return firstInstalled(bodyFamilies); }
QString Theme::controlFamily() const { return scriptDisplay ? bodyFamily() : displayFamily(); }

int Theme::loadFonts(const QStringList& dirs)
{
    int n = 0;
    for (const QString& d : dirs) {
        QDir dir(d);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList({"*.ttf", "*.otf", "*.TTF", "*.OTF"}, QDir::Files);
        for (const QString& f : files) {
            if (QFontDatabase::addApplicationFont(dir.absoluteFilePath(f)) >= 0)
                n++;
        }
    }
    if (n > 0)
        qInfo() << n << "Schriftdateien geladen";
    return n;
}

// ---------------------------------------------------------------- Stylesheets

QString Theme::centralStyle() const
{
    return QString("#central { background-color: %1; }").arg(rgba(bg));
}

QString Theme::gearStyle() const
{
    return QString(
        "QToolButton { color: %1; background-color: %2; border: %3px solid %4;"
        " border-radius: %5px; font-size: 30px; }"
        "QToolButton:pressed { background-color: %6; }")
        .arg(rgba(text), rgba(panel)).arg(panelBorderWidth).arg(rgba(panelBorder))
        .arg(radius).arg(rgba(accent));
}

QString Theme::hintStyle() const
{
    return QString(
        "color: %1; background-color: %2; border: %3px solid %4; border-radius: %5px;"
        " padding: 12px 24px; font-family: \"%6\"; font-size: 20px;")
        .arg(rgba(text), rgba(panel)).arg(panelBorderWidth).arg(rgba(panelBorder))
        .arg(radius).arg(bodyFamily());
}

QString Theme::countdownOverlayStyle() const
{
    return QString("#countdownOverlay { background-color: %1; }").arg(rgba(overlay));
}

QString Theme::countdownCircleStyle(int diameter) const
{
    return QString("background-color: %1; border: %2px solid %3; border-radius: %4px;")
        .arg(rgba(countdownFill)).arg(countdownRingWidth).arg(rgba(countdownRing))
        .arg(diameter / 2);
}

QString Theme::countdownLabelStyle() const
{
    return QString("color: %1; background: transparent; border: none; font-family: \"%2\";")
        .arg(rgba(countdownText), controlFamily());
}

QString Theme::countdownCaptionStyle() const
{
    return QString(
        "color: %1; background-color: %2; border-radius: %3px; padding: 8px 22px;"
        " font-family: \"%4\"; font-size: %5px;")
        .arg(rgba(text), rgba(panel)).arg(radius)
        .arg(scriptDisplay ? displayFamily() : bodyFamily()).arg(scriptDisplay ? 40 : 28);
}

QString Theme::messageStyle() const
{
    return QString(
        "color: %1; background-color: %2; padding: 8px 14px; border-radius: %3px;"
        " font-family: \"%4\"; font-size: 18px;")
        .arg(rgba(accentText), rgba(accent)).arg(radius).arg(bodyFamily());
}

QString Theme::qrCardStyle() const
{
    const QString title = upperCase ? "text-transform: uppercase;" : "";
    return QString(
        "#qrCard { background-color: %1; border: %2px solid %3; border-radius: %4px; }"
        "QLabel { background: transparent; border: none; }"
        "QLabel#qrTitle { color: %5; font-family: \"%6\"; font-size: %16px; %7 }"
        "QLabel#qrLink { color: %8; font-family: \"%9\"; font-size: 14px; }"
        "QLabel#qrPasswordLabel { color: %8; font-family: \"%9\"; font-size: 11px; letter-spacing: 2px; }"
        "QLabel#qrPassword { color: %10; background-color: %11; border: %12px solid %13;"
        "  border-radius: %14px; padding: 4px 8px; font-family: \"%15\"; font-size: 15px; font-weight: bold; }")
        .arg(rgba(panel)).arg(panelBorderWidth).arg(rgba(panelBorder)).arg(radius)
        .arg(rgba(text), displayFamily(), title)
        .arg(rgba(textMuted), bodyFamily())
        .arg(rgba(passwordText), rgba(passwordBg)).arg(passwordBorderWidth).arg(rgba(passwordBorder))
        .arg(qMin(radius, 6)).arg(controlFamily()).arg(scriptDisplay ? 32 : 24);
}

QString Theme::settingsStyle() const
{
    const QString title = upperCase ? "text-transform: uppercase;" : "";
    return QString(
        "#settingsPanel { background-color: %1; border: %2px solid %3; border-radius: %4px; }"
        "QLabel { color: %5; background: transparent; border: none; font-family: \"%6\"; }"
        "QLabel#title { font-family: \"%7\"; font-size: %19px; %8 }"
        "QLabel#rowTitle { font-size: 18px; }"
        "QLabel#rowValue { font-size: 18px; min-width: 70px; font-family: \"%20\"; }"
        "QCheckBox { color: %5; font-size: 18px; spacing: 12px; background: transparent; font-family: \"%6\"; }"
        "QCheckBox::indicator { width: 26px; height: 26px; border: 3px solid %3; border-radius: 4px; background: %1; }"
        "QCheckBox::indicator:checked { background: %9; }"
        "QSlider::groove:horizontal { height: 8px; background: %10; border-radius: 4px; }"
        "QSlider::sub-page:horizontal { background: %11; border-radius: 4px; }"
        "QSlider::handle:horizontal { width: 40px; height: 40px; margin: -16px 0; background: %12;"
        "  border: %13px solid %14; border-radius: 20px; }"
        "QComboBox { color: %5; background-color: %1; border: %2px solid %3; border-radius: %4px;"
        "  padding: 0 14px; min-height: 44px; font-family: \"%6\"; font-size: 18px; }"
        "QComboBox::drop-down { border: none; width: 40px; }"
        "QPushButton { color: %16; background-color: %17; border: %2px solid %18; border-radius: %4px;"
        "  padding: 12px 26px; font-family: \"%20\"; font-size: 17px; %8 }"
        "QPushButton#primary { color: %15; background-color: %9; border-color: %3; }"
        "QPushButton:pressed { background-color: %11; }")
        .arg(rgba(panel)).arg(panelBorderWidth).arg(rgba(panelBorder)).arg(radius)
        .arg(rgba(text), bodyFamily(), displayFamily(), title)
        .arg(rgba(primaryBg), rgba(sliderTrack), rgba(sliderFill), rgba(sliderHandle))
        .arg(sliderHandleBorderWidth).arg(rgba(sliderHandleBorder))
        .arg(rgba(primaryText), rgba(secondaryText), rgba(secondaryBg), rgba(secondaryBorder))
        .arg(scriptDisplay ? 40 : 32).arg(controlFamily());
}

QString Theme::comboPopupStyle() const
{
    QColor opaque = panel;
    opaque.setAlpha(255);
    return QString(
        "QWidget { background-color: %1; }"
        "QListView { color: %2; background-color: %1; border: %3px solid %4; outline: none;"
        "  font-family: \"%5\"; font-size: 18px; }"
        "QListView::item { min-height: 44px; padding: 0 12px; color: %2; background-color: %1; }"
        "QListView::item:hover { background-color: %6; color: %7; }"
        "QListView::item:selected { background-color: %6; color: %7; }")
        .arg(rgba(opaque), rgba(text)).arg(panelBorderWidth).arg(rgba(panelBorder))
        .arg(bodyFamily(), rgba(primaryBg), rgba(primaryText));
}

QString Theme::shadowStyle() const
{
    return QString("background-color: %1; border-radius: %2px;").arg(rgba(shadowColor)).arg(radius);
}

// ---------------------------------------------------------------- Themes

static Theme retro()
{
    Theme t;
    t.id = "retro";
    t.name = "Retro Photobooth";
    t.displayFamilies = {"Archivo Black", "Impact", "Arial Black", "Helvetica"};
    t.bodyFamilies = {"Special Elite", "American Typewriter", "Courier New", "Courier"};

    const QColor black(26, 26, 26), paper(242, 232, 201), red(230, 59, 46);
    t.bg = QColor(20, 20, 20);
    t.overlay = QColor(20, 20, 20, 150);
    t.panel = paper;
    t.panelBorder = black;
    t.panelBorderWidth = 3;
    t.radius = 4;
    t.text = black;
    t.textMuted = QColor(74, 63, 48);
    t.accent = red;
    t.accentText = paper;
    t.countdownFill = red;
    t.countdownRing = paper;
    t.countdownRingWidth = 10;
    t.countdownText = paper;
    t.sliderTrack = black;
    t.sliderFill = red;
    t.sliderHandle = paper;
    t.sliderHandleBorder = black;
    t.sliderHandleBorderWidth = 4;
    t.primaryBg = red;
    t.primaryText = paper;
    t.secondaryBg = paper;
    t.secondaryText = black;
    t.secondaryBorder = black;
    t.passwordBg = QColor(0, 0, 0, 0);
    t.passwordBorder = red;
    t.passwordBorderWidth = 3;
    t.passwordText = red;
    t.hardShadow = true;
    t.shadowColor = red;
    t.qrOnLeft = true;
    t.upperCase = true;
    t.hintText = QString::fromUtf8("Drück den roten Knopf!");
    t.countdownCaption = "Sag Cheese!";
    t.qrTitle = "Fotos ansehen";
    return t;
}

static Theme neon()
{
    Theme t;
    t.id = "neon";
    t.name = "Neon Party";
    t.displayFamilies = {"Bebas Neue", "Impact", "Arial Narrow", "Helvetica"};
    t.bodyFamilies = {"Rubik", "Helvetica Neue", "Arial", "Helvetica"};

    const QColor dark(11, 11, 18), pink(255, 63, 164), cyan(46, 230, 255), white(255, 255, 255);
    t.bg = dark;
    t.overlay = QColor(11, 11, 18, 140);
    t.panel = QColor(11, 11, 18, 220);
    t.panelBorder = QColor(46, 230, 255, 140);
    t.panelBorderWidth = 1;
    t.radius = 16;
    t.text = white;
    t.textMuted = QColor(255, 255, 255, 180);
    t.accent = pink;
    t.accentText = dark;
    t.countdownFill = QColor(0, 0, 0, 0);
    t.countdownRing = pink;
    t.countdownRingWidth = 6;
    t.countdownText = white;
    t.sliderTrack = QColor(42, 42, 58);
    t.sliderFill = pink;
    t.sliderHandle = white;
    t.sliderHandleBorder = white;
    t.sliderHandleBorderWidth = 0;
    t.primaryBg = cyan;
    t.primaryText = dark;
    t.secondaryBg = QColor(0, 0, 0, 0);
    t.secondaryText = white;
    t.secondaryBorder = QColor(255, 255, 255, 70);
    t.passwordBg = pink;
    t.passwordBorder = pink;
    t.passwordBorderWidth = 0;
    t.passwordText = dark;
    t.hardShadow = false;
    t.qrOnLeft = false;
    t.upperCase = true;
    t.hintText = QString::fromUtf8("Drück den Knopf für ein Foto");
    t.countdownCaption = QString::fromUtf8("Lächeln!");
    t.qrTitle = "Fotos ansehen";
    return t;
}

static Theme elegant()
{
    Theme t;
    t.id = "elegant";
    t.name = "Elegant";
    t.displayFamilies = {"Cormorant Garamond", "Didot", "Georgia", "Times New Roman"};
    t.bodyFamilies = {"Lato", "Gill Sans", "Helvetica Neue", "Helvetica"};

    const QColor cream(246, 241, 232), gold(184, 146, 90), ink(43, 38, 34);
    t.bg = QColor(21, 19, 15);
    t.overlay = QColor(21, 19, 15, 130);
    t.panel = cream;
    t.panelBorder = gold;
    t.panelBorderWidth = 1;
    t.radius = 2;
    t.text = ink;
    t.textMuted = QColor(90, 79, 68);
    t.accent = gold;
    t.accentText = cream;
    t.countdownFill = QColor(0, 0, 0, 0);
    t.countdownRing = cream;
    t.countdownRingWidth = 2;
    t.countdownText = cream;
    t.sliderTrack = QColor(217, 205, 187);
    t.sliderFill = gold;
    t.sliderHandle = cream;
    t.sliderHandleBorder = gold;
    t.sliderHandleBorderWidth = 2;
    t.primaryBg = ink;
    t.primaryText = cream;
    t.secondaryBg = QColor(0, 0, 0, 0);
    t.secondaryText = ink;
    t.secondaryBorder = QColor(0, 0, 0, 0);
    t.passwordBg = QColor(0, 0, 0, 0);
    t.passwordBorder = QColor(0, 0, 0, 0);
    t.passwordBorderWidth = 0;
    t.passwordText = ink;
    t.hardShadow = false;
    t.qrOnLeft = false;
    t.upperCase = false;
    t.hintText = QString::fromUtf8("Knopf drücken für ein Foto");
    t.countdownCaption = QString::fromUtf8("Bitte lächeln");
    t.qrTitle = "Eure Fotos";
    return t;
}

// Hochzeit in Rosé: weiche Blush-Töne, weisse Karten, geschwungene Schrift.
static Theme weddingBlush()
{
    Theme t;
    t.id = "wedding-blush";
    t.name = QString::fromUtf8("Hochzeit Rosé");
    t.displayFamilies = {"Great Vibes", "Snell Roundhand", "Apple Chancery", "Brush Script MT", "Georgia"};
    t.bodyFamilies = {"Montserrat", "Avenir Next", "Avenir", "Helvetica Neue", "Helvetica"};

    const QColor white(255, 252, 250), rose(196, 122, 132), deepRose(140, 74, 86), ink(70, 52, 56);
    t.bg = QColor(38, 28, 30);
    t.overlay = QColor(60, 36, 42, 140);
    t.panel = white;
    t.panelBorder = QColor(232, 196, 200);
    t.panelBorderWidth = 2;
    t.radius = 18;
    t.text = ink;
    t.textMuted = QColor(130, 100, 106);
    t.accent = rose;
    t.accentText = white;
    t.countdownFill = white;
    t.countdownRing = rose;
    t.countdownRingWidth = 4;
    t.countdownText = deepRose;
    t.sliderTrack = QColor(240, 220, 222);
    t.sliderFill = rose;
    t.sliderHandle = white;
    t.sliderHandleBorder = rose;
    t.sliderHandleBorderWidth = 3;
    t.primaryBg = rose;
    t.primaryText = white;
    t.secondaryBg = QColor(0, 0, 0, 0);
    t.secondaryText = deepRose;
    t.secondaryBorder = QColor(232, 196, 200);
    t.passwordBg = QColor(250, 232, 234);
    t.passwordBorder = QColor(232, 196, 200);
    t.passwordBorderWidth = 1;
    t.passwordText = deepRose;
    t.hardShadow = false;
    t.qrOnLeft = false;
    t.upperCase = false;
    t.hintText = QString::fromUtf8("Knopf drücken für ein Foto");
    t.countdownCaption = QString::fromUtf8("Bitte lächeln");
    t.qrTitle = "Unsere Fotos";
    t.scriptDisplay = true;
    return t;
}

// Hochzeit botanisch: Salbeigruen, Elfenbein, klassische Serife.
static Theme weddingGreen()
{
    Theme t;
    t.id = "wedding-green";
    t.name = "Hochzeit Botanik";
    t.displayFamilies = {"Playfair Display", "Baskerville", "Big Caslon", "Georgia", "Times New Roman"};
    t.bodyFamilies = {"Lato", "Avenir Next", "Gill Sans", "Helvetica Neue", "Helvetica"};

    const QColor ivory(248, 246, 239), sage(143, 168, 145), forest(63, 95, 74), ink(46, 56, 48);
    t.bg = QColor(24, 32, 27);
    t.overlay = QColor(30, 42, 34, 150);
    t.panel = ivory;
    t.panelBorder = sage;
    t.panelBorderWidth = 2;
    t.radius = 6;
    t.text = ink;
    t.textMuted = QColor(95, 110, 98);
    t.accent = forest;
    t.accentText = ivory;
    t.countdownFill = QColor(0, 0, 0, 0);
    t.countdownRing = sage;
    t.countdownRingWidth = 8;
    t.countdownText = ivory;
    t.sliderTrack = QColor(214, 222, 210);
    t.sliderFill = sage;
    t.sliderHandle = ivory;
    t.sliderHandleBorder = forest;
    t.sliderHandleBorderWidth = 3;
    t.primaryBg = forest;
    t.primaryText = ivory;
    t.secondaryBg = QColor(0, 0, 0, 0);
    t.secondaryText = forest;
    t.secondaryBorder = sage;
    t.passwordBg = QColor(0, 0, 0, 0);
    t.passwordBorder = sage;
    t.passwordBorderWidth = 2;
    t.passwordText = forest;
    t.hardShadow = false;
    t.qrOnLeft = false;
    t.upperCase = false;
    t.hintText = QString::fromUtf8("Knopf drücken für ein Foto");
    t.countdownCaption = QString::fromUtf8("Bitte lächeln");
    t.qrTitle = "Eure Fotos";
    return t;
}

// Pool Party: kraeftiges Blau, Tuerkis und Sonnengelb, runde Formen.
static Theme pool()
{
    Theme t;
    t.id = "pool";
    t.name = "Pool Party";
    t.displayFamilies = {"Fredoka", "Arial Rounded MT Bold", "Chalkboard SE", "Helvetica"};
    t.bodyFamilies = {"Nunito", "Avenir Next", "Avenir", "Helvetica Neue", "Helvetica"};

    const QColor navy(12, 52, 92), aqua(22, 193, 217), yellow(255, 210, 63), white(255, 255, 255);
    t.bg = QColor(11, 111, 163);
    t.overlay = QColor(12, 52, 92, 150);
    t.panel = white;
    t.panelBorder = aqua;
    t.panelBorderWidth = 4;
    t.radius = 22;
    t.text = navy;
    t.textMuted = QColor(70, 110, 140);
    t.accent = yellow;
    t.accentText = navy;
    t.countdownFill = aqua;
    t.countdownRing = white;
    t.countdownRingWidth = 10;
    t.countdownText = white;
    t.sliderTrack = QColor(210, 236, 244);
    t.sliderFill = aqua;
    t.sliderHandle = yellow;
    t.sliderHandleBorder = white;
    t.sliderHandleBorderWidth = 3;
    t.primaryBg = yellow;
    t.primaryText = navy;
    t.secondaryBg = QColor(210, 236, 244);
    t.secondaryText = navy;
    t.secondaryBorder = QColor(210, 236, 244);
    t.passwordBg = yellow;
    t.passwordBorder = yellow;
    t.passwordBorderWidth = 0;
    t.passwordText = navy;
    t.hardShadow = false;
    t.qrOnLeft = true;
    t.upperCase = false;
    t.hintText = QString::fromUtf8("Drück den Knopf!");
    t.countdownCaption = "Splash!";
    t.qrTitle = "Fotos vom Pool";
    return t;
}

QList<Theme> Theme::all()
{
    return {retro(), neon(), elegant(), weddingBlush(), weddingGreen(), pool()};
}

Theme Theme::byId(const QString& id)
{
    for (const Theme& t : all())
        if (t.id == id)
            return t;
    qWarning() << "Unbekanntes Theme" << id << "- nutze retro";
    return retro();
}
