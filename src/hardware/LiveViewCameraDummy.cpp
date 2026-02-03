#include "LiveViewCameraDummy.h"
#include <QPainter>
#include <QDateTime>

LiveViewCameraDummy::LiveViewCameraDummy(QObject* parent)
    : LiveViewCamera(parent)
{
    timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout,
            this, &LiveViewCameraDummy::generate);
}

void LiveViewCameraDummy::start()
{
    timer->start();
}

void LiveViewCameraDummy::stop()
{
    timer->stop();
}

void LiveViewCameraDummy::generate()
{
    QImage img(800, 600, QImage::Format_RGB32);
    img.fill(Qt::black);

    QPainter p(&img);
    p.setPen(Qt::green);
    p.setFont(QFont("Sans", 24));
    p.drawText(img.rect(),
               Qt::AlignCenter,
               QDateTime::currentDateTime().toString("hh:mm:ss"));

    emit frameReady(img);
}
