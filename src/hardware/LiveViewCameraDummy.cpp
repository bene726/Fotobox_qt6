#include "LiveViewCameraDummy.h"

#include <QDateTime>
#include <QPainter>
#include <QtMath>

LiveViewCameraDummy::LiveViewCameraDummy(QObject* parent)
    : LiveViewCamera(parent)
{
    timer = new QTimer(this);
    timer->setInterval(66); // ~15 fps
    connect(timer, &QTimer::timeout, this, &LiveViewCameraDummy::generate);
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
    QImage img(1280, 720, QImage::Format_RGB32);
    img.fill(QColor(20, 20, 20));

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    // Ein wandernder Kreis, damit man sieht, dass das Bild lebt.
    const double t = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    const int cx = int(img.width() / 2 + qCos(t) * img.width() / 3);
    const int cy = int(img.height() / 2 + qSin(t * 1.3) * img.height() / 3);
    p.setBrush(QColor(0, 160, 90));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(cx, cy), 60, 60);

    p.setPen(Qt::green);
    { QFont f; f.setPointSize(28); p.setFont(f); }
    p.drawText(img.rect(), Qt::AlignCenter,
               "Dummy Livebild\n" + QDateTime::currentDateTime().toString("hh:mm:ss"));

    emit frameReady(img);
}
