#pragma once
#include <QObject>
#include <QImage>

class LiveViewCamera : public QObject
{
    Q_OBJECT
public:
    explicit LiveViewCamera(QObject* parent = nullptr);
    virtual ~LiveViewCamera();

    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void frameReady(const QImage& frame);
};

LiveViewCamera* createLiveViewCamera();
