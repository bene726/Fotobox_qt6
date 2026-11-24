#pragma once
#include "Camera.h"
#include <QProcess>
#include <QTimer>

// DSLR ueber das Kommandozeilen-Tool gphoto2.
// Laeuft als eigener Prozess, blockiert das UI nicht.
class CameraGPhoto : public Camera
{
    Q_OBJECT
public:
    explicit CameraGPhoto(const QString& binary, QObject* parent = nullptr);
    ~CameraGPhoto() override;

    void capture(const QString& targetDir) override;

    // Uebersetzt eine Meldung von gphoto2 in einen Satz, der auf dem Display
    // der Fotobox weiterhilft. Oeffentlich, damit die Zuordnung gegen echte
    // gphoto2-Ausgaben geprueft werden kann - ohne Kamera.
    static QString friendlyError(const QString& raw);

private:
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError error);
    void fail(const QString& msg);

    QString m_binary;
    QString m_file;
    QProcess m_proc;
    bool m_busy = false;

    // Antwortet die Kamera gar nicht, haengt die Fotobox sonst fuer immer.
    // Eine Aufnahme dauert normal rund drei Sekunden.
    static constexpr int kTimeoutMs = 25000;
    QTimer m_timeout;
};
