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
    void requestBatteryLevel() override;

    // Liest den Prozentwert aus der Ausgabe von "gphoto2 --get-config
    // batterylevel". -1, wenn die Kamera nichts Brauchbares meldet.
    static int parseBatteryLevel(const QString& output);

private:
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError error);
    void fail(const QString& msg);

    QString m_binary;
    QString m_file;
    QProcess m_proc;
    bool m_busy = false;

    // Eigener Prozess fuer die Abfrage: der Aufnahmeprozess darf dabei nicht
    // gestoert werden, und gphoto2 belegt die Kamera jeweils allein.
    QProcess m_info;

    // Antwortet die Kamera gar nicht, haengt die Fotobox sonst fuer immer.
    // Eine Aufnahme dauert normal rund drei Sekunden.
    static constexpr int kTimeoutMs = 25000;
    // Die Abfrage ist ein kurzer Befehl; antwortet sie nicht, ist die Kamera weg.
    static constexpr int kInfoTimeoutMs = 10000;
    QTimer m_timeout;
};
