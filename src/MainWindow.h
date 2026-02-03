#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QStringList>
#include <memory>

#include "core/Config.h"

class QLabel;
class QrCodeWidget;
class Camera;
class TriggerThread;
class FileWatcher;
class LiveViewCamera;
class TouchInput;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onTriggered();           // Button
    void onTouched();             // Touch
    void onNewImage(const QString& path);

private:
    enum class UiMode {
        Slideshow,
        Countdown,
        ShowPhoto,
        LiveView
    };

    void setupUi();
    void startSlideshow();
    void showNextSlide();
    void startCountdown();

    // UI
    QLabel* imageView;
    QLabel* slideshowOverlay;
    QrCodeWidget* qrWidget;

    QWidget* overlay;
    QLabel* countdownLabel;

    // Timers
    QTimer* slideshowTimer;
    QTimer* countdownTimer;
    QTimer* liveViewTimeoutTimer;

    // State
    UiMode uiMode = UiMode::Slideshow;
    bool countdownActive = false;
    int countdownValue = 0;

    QStringList slideshowImages;
    int slideshowIndex = 0;

    // Core
    Config cfg;
    QString imagePath;

    // Hardware / Input
    std::unique_ptr<Camera> camera;
    LiveViewCamera* liveCam;
    TriggerThread* trigger;
    TouchInput* touch;
    FileWatcher* watcher;
};
