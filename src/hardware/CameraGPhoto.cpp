#include "CameraGPhoto.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>

CameraGPhoto::CameraGPhoto(const QString& binary, QObject* parent)
    : Camera(parent)
    , m_binary(binary)
{
    connect(&m_proc, &QProcess::finished, this, &CameraGPhoto::onFinished);
    connect(&m_proc, &QProcess::errorOccurred, this, &CameraGPhoto::onError);
}

CameraGPhoto::~CameraGPhoto()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.kill();
        m_proc.waitForFinished(2000);
    }
}

void CameraGPhoto::capture(const QString& targetDir)
{
    if (m_busy) {
        emit captureFailed("Aufnahme laeuft bereits");
        return;
    }

    QDir().mkpath(targetDir);
    m_file = targetDir + "/photo_"
             + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".jpg";
    m_busy = true;

    qInfo() << "gphoto2 Aufnahme nach" << m_file;
    m_proc.start(m_binary, {"--capture-image-and-download",
                            "--filename", m_file,
                            "--force-overwrite"});
}

void CameraGPhoto::fail(const QString& msg)
{
    if (!m_busy)
        return;
    m_busy = false;
    qWarning() << "gphoto2:" << msg;
    emit captureFailed(msg);
}

void CameraGPhoto::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        fail("gphoto2 nicht gefunden: " + m_binary);
    // Andere Fehler landen in onFinished().
}

void CameraGPhoto::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0) {
        QString err = QString::fromUtf8(m_proc.readAllStandardError()).trimmed();
        if (err.isEmpty())
            err = "gphoto2 Exit-Code " + QString::number(exitCode);
        fail(err);
        return;
    }

    if (!QFile::exists(m_file)) {
        fail("gphoto2 hat keine Datei geliefert: " + m_file);
        return;
    }

    m_busy = false;
    emit captured(m_file);
}
