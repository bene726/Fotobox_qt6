#include "MainWindow.h"

#include "hardware/Camera.h"
#include "hardware/LiveViewCamera.h"
#include "hardware/TriggerThread.h"
#include "input/TouchInputQt.h"
#include "core/FileWatcher.h"
#include "ui/QrCodeWidget.h"

#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QDir>
#include <QPainter>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    cfg = Config::load("/home/bene/Nextcloud_Fotobox/Config.js");
    imagePath = cfg.fullImagePath();

    camera.reset(createCamera());
    liveCam = createLiveViewCamera();

    setupUi();

    // Livebild immer aktiv
    connect(liveCam, &LiveViewCamera::frameReady,
            this, [this](const QImage& img) {
                imageView->setPixmap(
                    QPixmap::fromImage(img).scaled(
                        imageView->size(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation));
            });
    liveCam->start();

    // Touch
    touch = new TouchInputQt(this);
    qApp->installEventFilter(static_cast<TouchInputQt*>(touch));
    connect(touch, &TouchInput::touched,
            this, &MainWindow::onTouched);

    // Trigger
    trigger = new TriggerThread();
    connect(trigger, &TriggerThread::triggered,
            this, &MainWindow::onTriggered);
    trigger->start();

    // FileWatcher
    watcher = new FileWatcher(imagePath);
    connect(watcher, &FileWatcher::newFile,
            this, &MainWindow::onNewImage);

    startSlideshow();
}

MainWindow::~MainWindow()
{
    trigger->requestInterruption();
    trigger->quit();
    trigger->wait();
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    imageView = new QLabel(this);
    imageView->setMinimumSize(800, 600);
    imageView->setAlignment(Qt::AlignCenter);

    slideshowOverlay = new QLabel(imageView);
    slideshowOverlay->setAlignment(Qt::AlignCenter);
    slideshowOverlay->setStyleSheet("background-color: rgba(0,0,0,180); color:white;");
    slideshowOverlay->hide();

    qrWidget = new QrCodeWidget(cfg.link, this);
    qrWidget->setMinimumHeight(200);

    layout->addWidget(imageView, 1);
    layout->addWidget(qrWidget, 0);

    setCentralWidget(central);
    showFullScreen();

    // Countdown overlay
    overlay = new QWidget(imageView);
    overlay->setStyleSheet("background-color: rgba(0,0,0,160);");
    overlay->hide();

    countdownLabel = new QLabel(overlay);
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet("color:white;");
    countdownLabel->setFont(QFont("Sans", 96, QFont::Bold));

    countdownTimer = new QTimer(this);
    countdownTimer->setInterval(1000);
    connect(countdownTimer, &QTimer::timeout, this, [this]() {
        countdownValue--;
        if (countdownValue <= 0) {
            countdownTimer->stop();
            overlay->hide();
            countdownActive = false;
            uiMode = UiMode::ShowPhoto;
            camera->capture(imagePath);
        } else {
            countdownLabel->setText(QString::number(countdownValue));
        }
    });

    slideshowTimer = new QTimer(this);
    slideshowTimer->setInterval(4000);
    connect(slideshowTimer, &QTimer::timeout,
            this, &MainWindow::showNextSlide);

    liveViewTimeoutTimer = new QTimer(this);
    liveViewTimeoutTimer->setSingleShot(true);
    connect(liveViewTimeoutTimer, &QTimer::timeout,
            this, &MainWindow::startSlideshow);
}

void MainWindow::onTouched()
{
    if (uiMode == UiMode::Slideshow) {
        slideshowTimer->stop();
        slideshowOverlay->hide();
        uiMode = UiMode::LiveView;
    }
}

void MainWindow::onTriggered()
{
    if (countdownActive)
        return;

    slideshowTimer->stop();
    slideshowOverlay->hide();
    liveViewTimeoutTimer->stop();

    startCountdown();
}

void MainWindow::startCountdown()
{
    uiMode = UiMode::Countdown;
    countdownActive = true;
    countdownValue = 3;

    overlay->setGeometry(imageView->rect());
    countdownLabel->setGeometry(overlay->rect());
    countdownLabel->setText(QString::number(countdownValue));
    overlay->show();

    countdownTimer->start();
}

void MainWindow::onNewImage(const QString& path)
{
    QPixmap pix(path);
    if (!pix.isNull()) {
        slideshowOverlay->setPixmap(
            pix.scaled(imageView->size(),
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation));
        slideshowOverlay->show();
    }

    if (uiMode == UiMode::ShowPhoto) {
        uiMode = UiMode::LiveView;
        QTimer::singleShot(3000, this, [this]() {
            slideshowOverlay->hide();
            liveViewTimeoutTimer->start(3 * 60 * 1000);
        });
    }
}

void MainWindow::startSlideshow()
{
    uiMode = UiMode::Slideshow;
    slideshowOverlay->show();

    QDir dir(imagePath);
    slideshowImages = dir.entryList(
        QStringList() << "*.jpg" << "*.png",
        QDir::Files,
        QDir::Time);

    slideshowIndex = 0;

    if (!slideshowImages.isEmpty())
        showNextSlide();

    slideshowTimer->start();
}

void MainWindow::showNextSlide()
{
    if (slideshowImages.isEmpty())
        return;

    QString file = imagePath + "/" + slideshowImages.at(slideshowIndex);
    QPixmap pix(file);
    if (!pix.isNull()) {
        slideshowOverlay->setPixmap(
            pix.scaled(imageView->size(),
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation));
    }

    slideshowIndex = (slideshowIndex + 1) % slideshowImages.size();
}
