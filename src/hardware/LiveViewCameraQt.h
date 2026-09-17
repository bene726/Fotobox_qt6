#pragma once
#include "LiveViewCamera.h"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>

// Webcam (z.B. Mac-Kamera) ueber Qt Multimedia. Fuer die Entwicklung am Host.
class LiveViewCameraQt : public LiveViewCamera
{
    Q_OBJECT
public:
    explicit LiveViewCameraQt(QObject* parent = nullptr);
    ~LiveViewCameraQt() override;

    static bool available();

    void start() override;
    void stop() override;

private:
    void startCamera();

    QCamera* m_camera = nullptr;
    QMediaCaptureSession m_session;
    QVideoSink m_sink;
};
