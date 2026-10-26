#pragma once
#include <QList>
#include <QPair>
#include <QWidget>

#include "core/Config.h"

struct Theme;
class QSlider;
class QLabel;
class QCheckBox;
class QComboBox;
class QPushButton;
class QTimer;

// Einstellungs-Overlay fuer den Touchscreen.
// Aenderungen werden sofort per Signal gemeldet, "Speichern" schreibt die config.json.
class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(const Config& cfg, QWidget* parent = nullptr);

    void setLedBrightness(int percent);
    // Liste als (Kennung, Anzeigename); ist sie leer, bleibt die Zeile verborgen.
    void setCameras(const QList<QPair<QString, QString>>& cams, const QString& current,
                    bool switchable);
    void setConfig(const Config& cfg);
    void applyTheme(const Theme& theme);
    void openThemeDropdown();   // fuer Screenshots
    void resetShutdownButton(); // Bestaetigung zuruecknehmen, z.B. beim Schliessen

signals:
    void themeChanged(const QString& id);
    void cameraChanged(const QString& device);
    void ledBrightnessChanged(int percent);
    void countdownSecondsChanged(int seconds);
    void photoDisplayMsChanged(int ms);
    void showQrChanged(bool show);
    void slideshowEnabledChanged(bool on);
    void slideshowDelayChanged(int ms);
    void saveRequested();
    void closeRequested();
    void shutdownRequested();   // zweites Antippen von "Ausschalten"

private:
    QWidget* makeRow(const QString& title, QWidget* control, QLabel* value = nullptr);

    QComboBox* m_theme = nullptr;
    QComboBox* m_camera = nullptr;
    QWidget* m_cameraRow = nullptr;
    QSlider* m_led = nullptr;
    QLabel* m_ledValue = nullptr;
    QSlider* m_countdown = nullptr;
    QLabel* m_countdownValue = nullptr;
    QSlider* m_photo = nullptr;
    QLabel* m_photoValue = nullptr;
    QCheckBox* m_showQr = nullptr;
    QCheckBox* m_slideshow = nullptr;
    QSlider* m_slideshowDelay = nullptr;
    QLabel* m_slideshowDelayValue = nullptr;
    QWidget* m_slideshowDelayRow = nullptr;
    QPushButton* m_shutdown = nullptr;
    QTimer* m_shutdownArm = nullptr;   // Bestaetigung laeuft nach ein paar Sekunden ab
};
