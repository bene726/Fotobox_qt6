#pragma once
#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>

// Ein Theme ist ein Satz von Farben, Schriften und Texten.
// Die Stylesheets fuer die einzelnen Widgets werden daraus erzeugt.
struct Theme
{
    QString id;
    QString name;

    // Schriften: Kandidatenlisten, die erste installierte wird genutzt.
    QStringList displayFamilies;
    QStringList bodyFamilies;

    // Farben
    QColor bg;
    QColor overlay;                 // Abdunkelung ueber dem Livebild
    QColor panel, panelBorder;
    int    panelBorderWidth = 1;
    int    radius = 8;
    QColor text, textMuted;
    QColor accent, accentText;
    QColor countdownFill, countdownRing, countdownText;
    int    countdownRingWidth = 6;
    QColor sliderTrack, sliderFill, sliderHandle, sliderHandleBorder;
    int    sliderHandleBorderWidth = 0;
    QColor primaryBg, primaryText;
    QColor secondaryBg, secondaryText, secondaryBorder;
    QColor passwordBg, passwordBorder, passwordText;
    int    passwordBorderWidth = 0;
    bool   hardShadow = false;      // harter versetzter Schatten hinter Karten
    QColor shadowColor;
    bool   qrOnLeft = false;
    bool   upperCase = false;       // Titel in Grossbuchstaben
    bool   scriptDisplay = false;   // Display-Schrift ist eine Schreibschrift: nur fuer Titel,
                                    // Werte/Buttons/Countdown-Zahl nutzen die Textschrift

    // Texte
    QString hintText;
    QString countdownCaption;
    QString qrTitle;

    QString displayFamily() const;
    QString bodyFamily() const;
    QString controlFamily() const;  // Schrift fuer Werte, Buttons, Passwort, Countdown-Zahl

    // Stylesheets
    QString centralStyle() const;
    QString gearStyle() const;
    QString hintStyle() const;
    QString countdownOverlayStyle() const;
    QString countdownCircleStyle(int diameter) const;
    QString countdownLabelStyle() const;
    QString countdownCaptionStyle() const;
    QString messageStyle() const;
    QString qrCardStyle() const;
    QString settingsStyle() const;
    QString comboPopupStyle() const;   // Liste des Design-Dropdowns (eigenes Fenster)
    QString shadowStyle() const;

    static QList<Theme> all();
    static Theme byId(const QString& id);

    // Laedt alle .ttf/.otf aus den Ordnern. Gibt die Anzahl geladener Dateien zurueck.
    static int loadFonts(const QStringList& dirs);
};
