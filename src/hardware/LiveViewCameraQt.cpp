#include "LiveViewCameraQt.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMediaDevices>
#include <QVideoFrame>

#if QT_CONFIG(permissions)
#include <QPermissions>
#endif

bool LiveViewCameraQt::available()
{
    return !QMediaDevices::videoInputs().isEmpty();
}

LiveViewCameraQt::LiveViewCameraQt(QObject* parent)
    : LiveViewCamera(parent)
{
    m_session.setVideoSink(&m_sink);
    connect(&m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
        if (!frame.isValid())
            return;
        QImage img = frame.toImage();
        if (!img.isNull())
            emit frameReady(img);
    });
}

LiveViewCameraQt::~LiveViewCameraQt()
{
    stop();
}

// Die QCamera wird erst angelegt, wenn die Berechtigung sicher erteilt ist,
// sonst meldet Qt Multimedia schon beim Anlegen "Access to camera not granted".
void LiveViewCameraQt::startCamera()
{
    if (!m_camera) {
        const QCameraDevice dev = QMediaDevices::defaultVideoInput();
        qInfo() << "Qt-Kamera:" << dev.description();
        m_camera = new QCamera(dev, this);
        m_session.setCamera(m_camera);
        connect(m_camera, &QCamera::errorOccurred, this,
                [](QCamera::Error, const QString& msg) {
                    qWarning() << "Qt-Kamera Fehler:" << msg;
                });
    }
    m_camera->start();
}

void LiveViewCameraQt::start()
{
#if QT_CONFIG(permissions)
    QCameraPermission perm;
    switch (qApp->checkPermission(perm)) {
    case Qt::PermissionStatus::Undetermined:
        qInfo() << "Frage Kamera-Berechtigung an ...";
        qApp->requestPermission(perm, this, [this](const QPermission& p) {
            if (p.status() == Qt::PermissionStatus::Granted) {
                startCamera();
            } else {
                qWarning() << "Kamera-Berechtigung verweigert. In den Systemeinstellungen"
                           << "unter Datenschutz > Kamera freigeben.";
            }
        });
        return;
    case Qt::PermissionStatus::Denied:
        qWarning() << "Kamera-Berechtigung verweigert. In den Systemeinstellungen"
                   << "unter Datenschutz > Kamera freigeben.";
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    startCamera();
}

void LiveViewCameraQt::stop()
{
    if (m_camera)
        m_camera->stop();
}
