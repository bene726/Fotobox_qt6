#include "LiveViewCamera.h"

LiveViewCamera::LiveViewCamera(QObject* parent)
    : QObject(parent)
{
    connect(this, &LiveViewCamera::frameReady, this,
            [this](const QImage& f) { m_lastFrame = f; });
}

LiveViewCamera::~LiveViewCamera() = default;
