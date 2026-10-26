#include "MainWindow.h"

#include "core/FileWatcher.h"
#include "hardware/Camera.h"
#include "hardware/HardwareFactory.h"
#include "hardware/LedController.h"
#include "hardware/LiveViewCamera.h"
#include "hardware/Trigger.h"
#include "input/TouchInputQt.h"
#include "hardware/CameraScan.h"
#include "ui/QrCard.h"
#include "ui/SettingsPanel.h"

#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QResizeEvent>
#include <QProcess>
#include <QPushButton>
#include <QToolButton>

MainWindow::MainWindow(const Config& config, QWidget* parent)
    : QMainWindow(parent)
    , cfg(config)
{
    imagePath = cfg.fullImagePath();
    qInfo() << "Bildordner:" << imagePath;

    setupUi();
    setupHardware();
    applyTheme(cfg.theme);
    enterSlideshow();
}

// ---------------------------------------------------------------- UI

void MainWindow::setupUi()
{
    setWindowTitle("Fotobox");

    central = new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);

    // Reihenfolge der Erzeugung = Stapelreihenfolge (unten nach oben).
    liveView = new QLabel(central);
    liveView->setAlignment(Qt::AlignCenter);

    photoOverlay = new QLabel(central);
    photoOverlay->setAlignment(Qt::AlignCenter);
    photoOverlay->setStyleSheet("background-color: black;");
    photoOverlay->hide();

    countdownOverlay = new QWidget(central);
    countdownOverlay->setObjectName("countdownOverlay");
    countdownOverlay->setAttribute(Qt::WA_StyledBackground, true);
    countdownOverlay->hide();

    countdownCircle = new QWidget(countdownOverlay);
    countdownCircle->setAttribute(Qt::WA_StyledBackground, true);
    countdownLabel = new QLabel(countdownCircle);
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownCaption = new QLabel(countdownOverlay);
    countdownCaption->setAlignment(Qt::AlignCenter);

    shutterButton = new QPushButton(central);
    shutterButton->setCursor(Qt::PointingHandCursor);
    shutterButton->setFocusPolicy(Qt::NoFocus);   // sonst reagiert er auf die Leertaste
    connect(shutterButton, &QPushButton::clicked, this, &MainWindow::onTriggered);

    qrShadow = new QWidget(central);
    qrShadow->setAttribute(Qt::WA_StyledBackground, true);
    qrShadow->setAttribute(Qt::WA_TransparentForMouseEvents);
    qrCard = new QrCard(cfg.link, cfg.password, cfg.qrSize, central);
    qrShadow->setVisible(cfg.showQr);
    qrCard->setVisible(cfg.showQr);

    settingsButton = new QToolButton(central);
    settingsButton->setText(QString::fromUtf8("⚙"));
    settingsButton->setCursor(Qt::PointingHandCursor);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);

    settingsShadow = new QWidget(central);
    settingsShadow->setAttribute(Qt::WA_StyledBackground, true);
    settingsShadow->hide();

    settingsPanel = new SettingsPanel(cfg, central);
    settingsPanel->hide();
    connect(settingsPanel, &SettingsPanel::themeChanged, this, &MainWindow::applyTheme);
    connect(settingsPanel, &SettingsPanel::cameraChanged, this, [this](const QString& dev) {
        cfg.liveViewDevice = dev;
        if (liveCam && liveCam->setDevice(dev)) {
            liveFrameSeen = false;
            liveWatchdog.start();   // meldet sich, falls die neue Kamera nichts liefert
        } else {
            showMessage(QString::fromUtf8("Kamerawechsel braucht das Backend \"qt\"."), 5000);
        }
    });
    connect(settingsPanel, &SettingsPanel::ledBrightnessChanged, this, [this](int v) {
        cfg.ledBrightness = v;
        led->setBrightness(v);
    });
    connect(settingsPanel, &SettingsPanel::countdownSecondsChanged, this,
            [this](int v) { cfg.countdownSeconds = v; });
    connect(settingsPanel, &SettingsPanel::photoDisplayMsChanged, this, [this](int v) {
        cfg.photoDisplayMs = v;
        photoTimer.setInterval(v);
    });
    connect(settingsPanel, &SettingsPanel::showQrChanged, this, [this](bool v) {
        cfg.showQr = v;
        qrCard->setVisible(v);
        qrShadow->setVisible(v && theme.hardShadow);
    });
    connect(settingsPanel, &SettingsPanel::saveRequested, this, &MainWindow::saveSettings);
    connect(settingsPanel, &SettingsPanel::closeRequested, this, &MainWindow::closeSettings);
    connect(settingsPanel, &SettingsPanel::shutdownRequested, this, &MainWindow::shutdownSystem);

    messageLabel = new QLabel(central);
    messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    messageLabel->hide();

    // Timer
    slideshowTimer.setInterval(cfg.slideshowIntervalMs);
    connect(&slideshowTimer, &QTimer::timeout, this, &MainWindow::showNextSlide);

    countdownTimer.setInterval(1000);
    connect(&countdownTimer, &QTimer::timeout, this, &MainWindow::onCountdownTick);

    liveViewTimeoutTimer.setSingleShot(true);
    liveViewTimeoutTimer.setInterval(cfg.liveViewTimeoutMs);
    connect(&liveViewTimeoutTimer, &QTimer::timeout, this, &MainWindow::enterSlideshow);

    photoTimer.setSingleShot(true);
    photoTimer.setInterval(cfg.photoDisplayMs);
    connect(&photoTimer, &QTimer::timeout, this, &MainWindow::enterLiveView);

    messageTimer.setSingleShot(true);
    connect(&messageTimer, &QTimer::timeout, messageLabel, &QWidget::hide);

    // Kommt nach dem Start gar kein Frame, liegt meist das falsche Backend an.
    // Ohne Hinweis bliebe der Bildschirm einfach schwarz.
    liveWatchdog.setSingleShot(true);
    liveWatchdog.setInterval(6000);
    connect(&liveWatchdog, &QTimer::timeout, this, [this] {
        if (liveFrameSeen)
            return;
        qWarning().noquote()
            << "Es kommt kein Livebild an.\n"
               "         Backend pruefen: \"liveView\" in der Konfiguration.\n"
               "           qt     = USB-Webcam  (Liste mit: fotobox --list-cameras)\n"
               "           rpicam = Raspberry-Pi-Kamera am CSI-Anschluss\n"
               "         Genauer nachsehen: fotobox --test-camera";
        showMessage(QString::fromUtf8("Kein Livebild. Einstellung \"liveView\" prüfen."), 15000);
    });
}

void MainWindow::setupHardware()
{
    liveCam = createLiveViewCamera(cfg, this);
    connect(liveCam, &LiveViewCamera::frameReady, this, &MainWindow::onLiveFrame);
    liveCam->start();
    liveWatchdog.start();

    led = createLed(cfg, this);
    settingsPanel->setLedBrightness(led->brightness());

    camera = createCamera(cfg, liveCam, this);
    connect(camera, &Camera::captured, this, &MainWindow::onCaptured);
    connect(camera, &Camera::captureFailed, this, &MainWindow::onCaptureFailed);

    touch = new TouchInputQt(this);
    connect(touch, &TouchInput::touched, this, &MainWindow::onTouched);

    trigger = createTrigger(cfg, this);
    connect(trigger, &Trigger::triggered, this, &MainWindow::onTriggered);

    watcher = new FileWatcher(imagePath, this);
    connect(watcher, &FileWatcher::newFile, this, &MainWindow::onNewFile);
}

void MainWindow::applyTheme(const QString& id)
{
    theme = Theme::byId(id);
    cfg.theme = theme.id;
    qInfo() << "Design:" << theme.name << "(" << theme.displayFamily() << "/" << theme.bodyFamily() << ")";

    central->setStyleSheet(theme.centralStyle());
    settingsButton->setStyleSheet(theme.gearStyle());
    shutterButton->setStyleSheet(theme.shutterStyle());
    shutterButton->setText(theme.shutterText);
    countdownOverlay->setStyleSheet(theme.countdownOverlayStyle());
    countdownLabel->setStyleSheet(theme.countdownLabelStyle());
    countdownCaption->setStyleSheet(theme.countdownCaptionStyle());
    countdownCaption->setText(theme.countdownCaption);
    messageLabel->setStyleSheet(theme.messageStyle());
    qrShadow->setStyleSheet(theme.shadowStyle());
    qrShadow->setVisible(cfg.showQr && theme.hardShadow);
    settingsShadow->setStyleSheet(theme.shadowStyle());
    settingsShadow->setVisible(settingsPanel->isVisible() && theme.hardShadow);
    qrCard->applyTheme(theme);
    settingsPanel->applyTheme(theme);

    // Stylesheets wirken erst nach dem Polish, daher zweimal layouten.
    layoutLayers();
    QTimer::singleShot(0, this, &MainWindow::layoutLayers);
}

void MainWindow::layoutLayers()
{
    const QRect r = central->rect();
    const int margin = 24;
    const int shadowOffset = 8;

    liveView->setGeometry(r);
    photoOverlay->setGeometry(r);

    // Countdown: Kreis in der Mitte, Untertitel darunter
    countdownOverlay->setGeometry(r);
    const int d = qMin(r.width(), r.height()) * 55 / 100;
    countdownCircle->setGeometry((r.width() - d) / 2, (r.height() - d) / 2 - 20, d, d);
    countdownCircle->setStyleSheet(theme.countdownCircleStyle(d));
    countdownLabel->setGeometry(countdownCircle->rect());
    QFont f = countdownLabel->font();
    f.setPixelSize(d * 70 / 100);
    f.setBold(true);
    countdownLabel->setFont(f);
    countdownCaption->adjustSize();
    countdownCaption->move((r.width() - countdownCaption->width()) / 2,
                           countdownCircle->geometry().bottom() + 18);

    // QR-Karte unten rechts
    qrCard->adjustSize();
    const QSize qs = qrCard->size();
    qrCard->setGeometry(r.width() - qs.width() - margin, r.height() - qs.height() - margin,
                        qs.width(), qs.height());
    qrShadow->setGeometry(qrCard->geometry().translated(shadowOffset, shadowOffset));

    // Ausloeser: im Bereich links neben der QR-Karte zentriert
    shutterButton->adjustSize();
    const QSize sb = shutterButton->sizeHint();
    const int area = cfg.showQr ? qrCard->geometry().left() - margin : r.width();
    shutterButton->setGeometry((area - sb.width()) / 2, r.height() - sb.height() - margin - 8,
                               sb.width(), sb.height());

    settingsButton->setGeometry(margin, margin, 56, 56);

    const int pw = qMin(r.width() - 2 * margin, 640);
    const int ph = qMin(r.height() - 2 * margin, 500);
    settingsPanel->setGeometry((r.width() - pw) / 2, (r.height() - ph) / 2, pw, ph);
    settingsShadow->setGeometry(settingsPanel->geometry().translated(shadowOffset, shadowOffset));

    const int msgH = 56;
    messageLabel->setGeometry(margin, margin + 56 + 16, r.width() - 2 * margin, msgH);

    if (!overlaySource.isNull())
        photoOverlay->setPixmap(overlaySource.scaled(
            r.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    layoutLayers();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Q) {
        close();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::showOverlayImage(const QPixmap& pix)
{
    overlaySource = pix;
    photoOverlay->setPixmap(pix.scaled(central->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
    photoOverlay->show();
}

void MainWindow::showMessage(const QString& text, int ms)
{
    qWarning() << text;
    messageLabel->setText(text);
    messageLabel->show();
    messageLabel->raise();
    messageTimer.start(ms);
}

void MainWindow::onLiveFrame(const QImage& frame)
{
    if (!liveFrameSeen) {
        liveFrameSeen = true;
        liveWatchdog.stop();
        qInfo() << "Erstes Livebild erhalten:" << frame.width() << "x" << frame.height();
    }
    liveView->setPixmap(QPixmap::fromImage(frame).scaled(
        liveView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// ---------------------------------------------------------------- Zustaende

const char* MainWindow::stateName(State s)
{
    switch (s) {
    case State::Slideshow: return "Slideshow";
    case State::LiveView:  return "LiveView";
    case State::Countdown: return "Countdown";
    case State::Capturing: return "Capturing";
    case State::ShowPhoto: return "ShowPhoto";
    }
    return "?";
}

void MainWindow::setState(State s)
{
    if (s == state)
        return;
    qInfo() << "Zustand:" << stateName(state) << "->" << stateName(s);
    state = s;
}

void MainWindow::enterSlideshow()
{
    setState(State::Slideshow);
    liveViewTimeoutTimer.stop();
    photoTimer.stop();
    shutterButton->show();

    slideshowImages = FileWatcher::imageFiles(imagePath);
    slideshowIndex = 0;

    if (slideshowImages.isEmpty()) {
        qInfo() << "Keine Bilder fuer die Slideshow, zeige Livebild.";
        photoOverlay->hide();
    } else {
        qInfo() << "Slideshow mit" << slideshowImages.size() << "Bildern";
        showNextSlide();
    }
    slideshowTimer.start();
}

void MainWindow::showNextSlide()
{
    if (state != State::Slideshow)
        return;

    if (slideshowImages.isEmpty()) {
        photoOverlay->hide();
        return;
    }

    if (slideshowIndex >= slideshowImages.size())
        slideshowIndex = 0;

    const QString file = slideshowImages.at(slideshowIndex);
    slideshowIndex = (slideshowIndex + 1) % slideshowImages.size();

    QPixmap pix(file);
    if (pix.isNull()) {
        qWarning() << "Bild nicht ladbar:" << file;
        return;
    }
    showOverlayImage(pix);
}

void MainWindow::enterLiveView()
{
    setState(State::LiveView);
    slideshowTimer.stop();
    photoTimer.stop();
    photoOverlay->hide();
    overlaySource = QPixmap();
    settingsButton->show();
    shutterButton->setVisible(!settingsPanel->isVisible());
    if (!settingsPanel->isVisible())
        liveViewTimeoutTimer.start();
}

void MainWindow::startCountdown()
{
    setState(State::Countdown);
    slideshowTimer.stop();
    liveViewTimeoutTimer.stop();
    photoTimer.stop();
    photoOverlay->hide();
    shutterButton->hide();
    settingsButton->hide();

    countdownValue = cfg.countdownSeconds;
    if (countdownValue <= 0) {
        onCountdownTick();
        return;
    }

    countdownLabel->setText(QString::number(countdownValue));
    countdownOverlay->show();
    countdownOverlay->raise();
    countdownTimer.start();
}

void MainWindow::showDemoState(const QString& which)
{
    if (which == "settings") {
        openSettings();
    } else if (which == "shutdown") {
        // Nur ein einziges Antippen: das zweite wuerde wirklich herunterfahren.
        openSettings();
        QTimer::singleShot(300, this, [this] {
            if (auto* b = settingsPanel->findChild<QPushButton*>("danger"))
                b->click();
        });
    } else if (which == "dropdown") {
        openSettings();
        QTimer::singleShot(300, settingsPanel, &SettingsPanel::openThemeDropdown);
    } else if (which == "countdown") {
        startCountdown();
        countdownTimer.stop(); // eingefroren bei der Startzahl
    } else if (which == "liveview") {
        enterLiveView();
    }
}

// ---------------------------------------------------------------- Einstellungen

void MainWindow::openSettings()
{
    if (state == State::Countdown || state == State::Capturing)
        return;
    enterLiveView();
    liveViewTimeoutTimer.stop();
    shutterButton->hide();
    settingsPanel->setConfig(cfg);
    settingsPanel->setCameras(CameraScan::devices(), cfg.liveViewDevice,
                              liveCam && liveCam->canSetDevice());
    settingsShadow->setVisible(theme.hardShadow);
    settingsPanel->show();
    settingsPanel->raise();
    messageLabel->raise();
}

void MainWindow::closeSettings()
{
    settingsPanel->resetShutdownButton();
    settingsPanel->hide();
    settingsShadow->hide();
    enterLiveView();
}

void MainWindow::saveSettings()
{
    if (cfg.save())
        showMessage("Einstellungen gespeichert", 2000);
    else
        showMessage("Speichern fehlgeschlagen: " + cfg.sourcePath, 5000);
    closeSettings();
}

void MainWindow::shutdownSystem()
{
    // systemctl poweroff ueber logind. Aus dem systemd-Benutzerdienst heraus
    // gibt es keine aktive Sitzung, daher erlaubt die polkit-Regel aus dem
    // Paket (50-fotobox-power.rules) das Ausschalten fuer die Gruppe fotobox.
    qInfo() << "Ausschalten angefordert";
    showMessage(QString::fromUtf8("Fotobox wird ausgeschaltet …"), 60000);

    auto* proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus status) {
        if (status != QProcess::NormalExit || code != 0) {
            const QString err = QString::fromLocal8Bit(proc->readAllStandardError()).trimmed();
            qWarning() << "Ausschalten fehlgeschlagen:" << code << err;
            showMessage("Ausschalten fehlgeschlagen: " + err, 8000);
            settingsPanel->resetShutdownButton();
        }
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            qWarning() << "systemctl nicht startbar";
            showMessage("Ausschalten fehlgeschlagen: systemctl fehlt", 8000);
            settingsPanel->resetShutdownButton();
            proc->deleteLater();
        }
    });
    proc->start("systemctl", {"poweroff"});
}

// ---------------------------------------------------------------- Ereignisse

void MainWindow::onTouched()
{
    if (settingsPanel->isVisible())
        return;
    switch (state) {
    case State::Slideshow:
        enterLiveView();
        break;
    case State::LiveView:
        // Aktivitaet haelt das Livebild wach.
        liveViewTimeoutTimer.start();
        break;
    default:
        break;
    }
}

void MainWindow::onTriggered()
{
    if (settingsPanel->isVisible()) {
        qInfo() << "Ausloeser ignoriert: Einstellungen offen";
        return;
    }
    if (state == State::Slideshow || state == State::LiveView) {
        startCountdown();
    } else {
        qInfo() << "Ausloeser ignoriert im Zustand" << stateName(state);
    }
}

void MainWindow::onCountdownTick()
{
    countdownValue--;
    if (countdownValue > 0) {
        countdownLabel->setText(QString::number(countdownValue));
        return;
    }

    countdownTimer.stop();
    countdownOverlay->hide();
    setState(State::Capturing);

    // Die Spiegelreflex braucht ein paar Sekunden zum Auslösen und Übertragen.
    // Solange das letzte Livebild einfrieren, damit sofort etwas zu sehen ist.
    // Das echte Foto ersetzt es, sobald es da ist.
    const QImage frozen = liveCam ? liveCam->lastFrame() : QImage();
    if (!frozen.isNull()) {
        qInfo() << "Livebild eingefroren, warte auf die Kamera";
        showOverlayImage(QPixmap::fromImage(frozen));
    }

    camera->capture(imagePath);
}

void MainWindow::onCaptured(const QString& path)
{
    qInfo() << "Foto aufgenommen:" << path;
    if (!slideshowImages.contains(path))
        slideshowImages << path;

    QPixmap pix(path);
    if (pix.isNull()) {
        showMessage("Foto konnte nicht geladen werden");
        enterLiveView();
        return;
    }

    setState(State::ShowPhoto);
    showOverlayImage(pix);
    photoTimer.start();
}

void MainWindow::onCaptureFailed(const QString& error)
{
    showMessage("Foto fehlgeschlagen: " + error, 5000);
    enterLiveView();
}

void MainWindow::onNewFile(const QString& path)
{
    if (!slideshowImages.contains(path))
        slideshowImages << path;
}
