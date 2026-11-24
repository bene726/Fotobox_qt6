#pragma once
#include "LiveViewCamera.h"
#include "core/FrameGate.h"

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
    // fps: Obergrenze fuer die verarbeiteten Bilder; 0 nimmt alles
    //
    // Die Bildrate laesst sich einer Webcam nicht vorgeben - sie liefert, was
    // ihr Format hergibt. Deshalb wird hier verworfen statt verlangt.
    LiveViewCameraQt(const QString& device = {}, int width = 0, int height = 0,
                     int fps = 0, QObject* parent = nullptr);
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
    FrameGate m_gate;
};
