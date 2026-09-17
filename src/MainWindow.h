#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QStringList>
#include <QTimer>

#include "core/Config.h"
#include "ui/Theme.h"

class QLabel;
class QrCard;
class Camera;
class Trigger;
class FileWatcher;
class LiveViewCamera;
class TouchInput;
class LedController;
class SettingsPanel;
class QToolButton;

// Zentrale Steuerung: kennt alle Zustaende und verbindet Hardware mit UI.
//
// Ebenen (von unten nach oben):
//   liveView         - Livebild, laeuft immer
//   photoOverlay     - Slideshow bzw. gerade aufgenommenes Foto
//   countdownOverlay - Countdown mit Kreis und Untertitel
//   hintLabel        - "Knopf druecken" unten links
//   qrCard           - QR-Code, Link, Passwort unten rechts
//   settingsButton   - Zahnrad oben links
//   settingsPanel    - Einstellungen (Design, LED, Countdown, ...)
//   messageLabel     - kurze Statusmeldungen
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const Config& cfg, QWidget* parent = nullptr);

    // Fuer Screenshots/Tests: "settings" oeffnet die Einstellungen,
    // "countdown" startet einen Countdown ohne Aufnahme.
    void showDemoState(const QString& which);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class State {
        Slideshow,   // Overlay mit alten Fotos, Livebild darunter
        LiveView,    // nur Livebild
        Countdown,   // 3..2..1
        Capturing,   // warten auf die Kamera
        ShowPhoto    // neues Foto wird angezeigt
    };
    static const char* stateName(State s);

    void setupUi();
    void setupHardware();
    void applyTheme(const QString& id);
    void layoutLayers();
    void setState(State s);

    // Zustandswechsel
    void enterSlideshow();
    void enterLiveView();
    void startCountdown();

    // Ereignisse
    void onTouched();
    void onTriggered();
    void onCountdownTick();
    void onCaptured(const QString& path);
    void onCaptureFailed(const QString& error);
    void onNewFile(const QString& path);
    void onLiveFrame(const QImage& frame);
    void showNextSlide();

    void openSettings();
    void closeSettings();
    void saveSettings();

    void showOverlayImage(const QPixmap& pix);
    void showMessage(const QString& text, int ms = 3000);

    Config cfg;
    Theme theme;
    QString imagePath;
    State state = State::Slideshow;

    // UI
    QWidget* central = nullptr;
    QLabel* liveView = nullptr;
    QLabel* photoOverlay = nullptr;
    QWidget* countdownOverlay = nullptr;
    QWidget* countdownCircle = nullptr;
    QLabel* countdownLabel = nullptr;
    QLabel* countdownCaption = nullptr;
    QLabel* hintLabel = nullptr;
    QWidget* qrShadow = nullptr;
    QrCard* qrCard = nullptr;
    QToolButton* settingsButton = nullptr;
    QWidget* settingsShadow = nullptr;
    SettingsPanel* settingsPanel = nullptr;
    QLabel* messageLabel = nullptr;
    QPixmap overlaySource;

    // Timer
    QTimer slideshowTimer;
    QTimer countdownTimer;
    QTimer liveViewTimeoutTimer;
    QTimer photoTimer;
    QTimer messageTimer;

    int countdownValue = 0;
    QStringList slideshowImages;
    int slideshowIndex = 0;

    // Hardware / Input
    Camera* camera = nullptr;
    LiveViewCamera* liveCam = nullptr;
    Trigger* trigger = nullptr;
    TouchInput* touch = nullptr;
    FileWatcher* watcher = nullptr;
    LedController* led = nullptr;
};
