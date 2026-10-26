#pragma once
#include "LiveViewCamera.h"

#include <QCamera>
#include <QSize>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QVideoSink>

// Webcam (z.B. Mac-Kamera) ueber Qt Multimedia. Fuer die Entwicklung am Host.
class LiveViewCameraQt : public LiveViewCamera
{
    Q_OBJECT
public:
    // device: Teil des Namens oder der Kennung; leer = Standardkamera
    // width/height: gewuenschte Aufloesung; die naechstgelegene wird genommen
    LiveViewCameraQt(const QString& device = {}, int width = 0, int height = 0,
                     QObject* parent = nullptr);
    ~LiveViewCameraQt() override;

    static bool available();

    // Index der am besten passenden Aufloesung, -1 wenn die Liste leer ist.
    // Als eigene Funktion, damit sie ohne Kamera pruefbar ist.
    static int pickNearest(const QList<QSize>& sizes, int width, int height);

    void start() override;
    void stop() override;
    bool setDevice(const QString& device) override;
    bool canSetDevice() const override { return true; }

private:
    QCameraDevice resolve(const QString& device) const;
    QCameraFormat chooseFormat() const;   // passend zu m_width/m_height
    void startCamera();

    QCameraDevice m_dev;   // beim Start ausgewaehlt
    int m_width = 0;
    int m_height = 0;
    QCameraFormat m_format;

    QCamera* m_camera = nullptr;
    QMediaCaptureSession m_session;
    QVideoSink m_sink;
};
