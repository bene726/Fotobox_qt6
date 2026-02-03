#include "QrCodeWidget.h"
#include "QrEncoder.h"

#include <QPainter>

QrCodeWidget::QrCodeWidget(const QString& t, QWidget* parent)
    : QWidget(parent)
{
    setText(t);
}

void QrCodeWidget::setText(const QString& t)
{
    text = t;
    qrImage = QrEncoderQt::encode(text, 6);
    update();
}

void QrCodeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    if (!qrImage.isNull()) {
        QSize s = qrImage.size();
        QPoint center = rect().center() - QPoint(s.width()/2, s.height()/2);
        p.drawImage(center, qrImage);
    }
}
