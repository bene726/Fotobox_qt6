#pragma once
#include <QWidget>

#include "core/Config.h"

struct Theme;
class QSlider;
class QLabel;
class QCheckBox;
class QComboBox;

// Einstellungs-Overlay fuer den Touchscreen.
// Aenderungen werden sofort per Signal gemeldet, "Speichern" schreibt die config.json.
class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(const Config& cfg, QWidget* parent = nullptr);

    void setLedBrightness(int percent);
    void setConfig(const Config& cfg);
    void applyTheme(const Theme& theme);
    void openThemeDropdown();   // fuer Screenshots

signals:
    void themeChanged(const QString& id);
    void ledBrightnessChanged(int percent);
    void countdownSecondsChanged(int seconds);
    void photoDisplayMsChanged(int ms);
    void showQrChanged(bool show);
    void saveRequested();
    void closeRequested();

private:
    QWidget* makeRow(const QString& title, QWidget* control, QLabel* value = nullptr);

    QComboBox* m_theme = nullptr;
    QSlider* m_led = nullptr;
    QLabel* m_ledValue = nullptr;
    QSlider* m_countdown = nullptr;
    QLabel* m_countdownValue = nullptr;
    QSlider* m_photo = nullptr;
    QLabel* m_photoValue = nullptr;
    QCheckBox* m_showQr = nullptr;
};
