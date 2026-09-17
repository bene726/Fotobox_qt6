#pragma once
#include "LiveViewCamera.h"
#include <QByteArray>
#include <QProcess>
#include <QTimer>

// Livebild der Raspberry-Pi-Kamera ueber rpicam-vid.
// rpicam-vid schreibt einen MJPEG-Stream auf stdout, hier werden
// die einzelnen JPEG-Frames herausgeschnitten und dekodiert.
//
// Noch nicht auf echter Hardware getestet (Phase 4).
class LiveViewCameraRpicam : public LiveViewCamera
{
    Q_OBJECT
public:
    LiveViewCameraRpicam(const QString& binary, int width, int height, int fps,
                         QObject* parent = nullptr);
    ~LiveViewCameraRpicam() override;

    void start() override;
    void stop() override;

private:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);

    QString m_binary;
    int m_width;
    int m_height;
    int m_fps;
    bool m_running = false;

    QProcess m_proc;
    QByteArray m_buffer;
    QTimer m_restart;
};
