#include "LiveViewCameraQt.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMediaDevices>
#include <QVideoFrame>

// QPermissions gibt es erst ab Qt 6.5.
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#if QT_CONFIG(permissions)
#define FOTOBOX_HAS_PERMISSIONS 1
#include <QPermissions>
#endif
#endif

bool LiveViewCameraQt::available()
{
    return !QMediaDevices::videoInputs().isEmpty();
}

LiveViewCameraQt::LiveViewCameraQt(const QString& device, QObject* parent)
    : LiveViewCamera(parent)
{
    // Die Kamera gleich hier waehlen, damit die Entscheidung auch dann im Log
    // steht, wenn das Starten spaeter an einer Berechtigung scheitert.
    m_dev = resolve(device);
    qInfo() << "Qt-Kamera:" << m_dev.description();

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
        m_camera = new QCamera(m_dev, this);
        m_session.setCamera(m_camera);
        connect(m_camera, &QCamera::errorOccurred, this,
                [](QCamera::Error, const QString& msg) {
                    qWarning() << "Qt-Kamera Fehler:" << msg;
                });
    }
    m_camera->start();
}

QCameraDevice LiveViewCameraQt::resolve(const QString& device) const
{
    if (device.isEmpty())
        return QMediaDevices::defaultVideoInput();
    for (const QCameraDevice& c : QMediaDevices::videoInputs()) {
        if (c.description().contains(device, Qt::CaseInsensitive)
            || QString::fromUtf8(c.id()).contains(device, Qt::CaseInsensitive))
            return c;
    }
    qWarning() << "Keine Kamera passt zu" << device
               << "- nutze die Standardkamera. Liste mit --list-cameras.";
    return QMediaDevices::defaultVideoInput();
}

bool LiveViewCameraQt::setDevice(const QString& device)
{
    const QCameraDevice dev = resolve(device);
    if (dev == m_dev)
        return true;

    m_dev = dev;
    qInfo() << "Wechsle auf Kamera:" << m_dev.description();
    if (m_camera) {
        m_camera->stop();
        m_camera->deleteLater();
        m_camera = nullptr;
    }
    startCamera();
    return true;
}

void LiveViewCameraQt::start()
{
#ifdef FOTOBOX_HAS_PERMISSIONS
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
