#include "CameraSnapshot.h"
#include "LiveViewCamera.h"

#include <QDateTime>
#include <QDir>
#include <QTimer>

CameraSnapshot::CameraSnapshot(LiveViewCamera* live, QObject* parent)
    : Camera(parent)
    , m_live(live)
{
}

void CameraSnapshot::capture(const QString& targetDir)
{
    const QImage img = m_live ? m_live->lastFrame() : QImage();
    if (img.isNull()) {
        emit captureFailed("Kein Livebild vorhanden");
        return;
    }

    QDir().mkpath(targetDir);
    const QString file = targetDir + "/snap_"
                         + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".jpg";
    if (!img.save(file, "JPG", 92)) {
        emit captureFailed("Bild konnte nicht gespeichert werden: " + file);
        return;
    }

    QTimer::singleShot(300, this, [this, file] { emit captured(file); });
}
