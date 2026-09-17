#include "HardwareFactory.h"

#include "core/Config.h"
#include "CameraDummy.h"
#include "CameraGPhoto.h"
#include "CameraSnapshot.h"
#include "LedController.h"
#include "LedPwm.h"
#include "LiveViewCameraDummy.h"
#include "LiveViewCameraRpicam.h"
#include "TriggerGpio.h"
#include "TriggerKeyboard.h"
#include "TriggerTimer.h"

#ifdef FOTOBOX_HAS_QTMULTIMEDIA
#include "LiveViewCameraQt.h"
#endif

#include <QDebug>

static QString resolve(const QString& value, const char* piDefault, const char* hostDefault)
{
    if (value != "auto")
        return value;
#ifdef FOTOBOX_PI
    Q_UNUSED(hostDefault);
    return piDefault;
#else
    Q_UNUSED(piDefault);
    return hostDefault;
#endif
}

LiveViewCamera* createLiveViewCamera(const Config& cfg, QObject* parent)
{
    QString b = resolve(cfg.liveViewBackend, "rpicam", "qt");

#ifdef FOTOBOX_HAS_QTMULTIMEDIA
    if (b == "qt" && !LiveViewCameraQt::available()) {
        qWarning() << "Keine Webcam gefunden, nutze Livebild-Dummy";
        b = "dummy";
    }
#else
    if (b == "qt") {
        qWarning() << "Qt Multimedia nicht eingebaut, nutze Livebild-Dummy";
        b = "dummy";
    }
#endif

    qInfo() << "Livebild-Backend:" << b;
    if (b == "rpicam")
        return new LiveViewCameraRpicam(cfg.rpicamBinary, cfg.liveViewWidth,
                                        cfg.liveViewHeight, cfg.liveViewFps, parent);
#ifdef FOTOBOX_HAS_QTMULTIMEDIA
    if (b == "qt")
        return new LiveViewCameraQt(parent);
#endif
    if (b != "dummy")
        qWarning() << "Unbekanntes Livebild-Backend" << b << "- nutze dummy";
    return new LiveViewCameraDummy(parent);
}

Camera* createCamera(const Config& cfg, LiveViewCamera* live, QObject* parent)
{
    const QString b = resolve(cfg.cameraBackend, "gphoto", "snapshot");
    qInfo() << "Kamera-Backend:" << b;
    if (b == "gphoto")
        return new CameraGPhoto(cfg.gphoto2Binary, parent);
    if (b == "snapshot")
        return new CameraSnapshot(live, parent);
    if (b != "dummy")
        qWarning() << "Unbekanntes Kamera-Backend" << b << "- nutze dummy";
    return new CameraDummy(parent);
}

Trigger* createTrigger(const Config& cfg, QObject* parent)
{
    const QString b = resolve(cfg.triggerBackend, "gpio", "keyboard");
    qInfo() << "Trigger-Backend:" << b;
    if (b == "gpio")
        return new TriggerGpio(cfg.gpioChip, cfg.gpioLine, parent);
    if (b == "timer")
        return new TriggerTimer(cfg.autoTriggerMs, parent);
    if (b != "keyboard")
        qWarning() << "Unbekanntes Trigger-Backend" << b << "- nutze keyboard";
    return new TriggerKeyboard(parent);
}

LedController* createLed(const Config& cfg, QObject* parent)
{
    const QString b = resolve(cfg.ledBackend, "pwm", "dummy");
    qInfo() << "LED-Backend:" << b;
    LedController* led = nullptr;
    if (b == "pwm")
        led = new LedPwm(cfg.ledPwmChip, cfg.ledPwmChannel, parent);
    else {
        if (b != "dummy")
            qWarning() << "Unbekanntes LED-Backend" << b << "- nutze dummy";
        led = new LedDummy(parent);
    }
    led->setBrightness(cfg.ledBrightness);
    return led;
}
