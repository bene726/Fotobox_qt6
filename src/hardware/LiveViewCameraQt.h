#pragma once
#include "LiveViewCamera.h"

#include <QCamera>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QVideoSink>

// Webcam (z.B. Mac-Kamera) ueber Qt Multimedia. Fuer die Entwicklung am Host.
class LiveViewCameraQt : public LiveViewCamera
{
    Q_OBJECT
public:
    // device: Teil des Namens oder der Kennung; leer = Standardkamera
    explicit LiveViewCameraQt(const QString& device = {}, QObject* parent = nullptr);
    ~LiveViewCameraQt() override;

    static bool available();

    void start() override;
    void stop() override;
    bool setDevice(const QString& device) override;
    bool canSetDevice() const override { return true; }

private:
    QCameraDevice resolve(const QString& device) const;
    void startCamera();

    QCameraDevice m_dev;   // beim Start ausgewaehlt

    QCamera* m_camera = nullptr;
    QMediaCaptureSession m_session;
    QVideoSink m_sink;
};
