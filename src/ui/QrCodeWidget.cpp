#include "QrCodeWidget.h"
#include "QrEncoder.h"

#include <QPainter>

QrCodeWidget::QrCodeWidget(const QString& t, QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setText(t);
}

void QrCodeWidget::setText(const QString& t)
{
    text = t;
    qrImage = QrEncoderQt::encode(text, 1);
    update();
}

void QrCodeWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(rect(), 8, 8);

    if (qrImage.isNull())
        return;

    const int margin = 10;
    const int side = qMin(width(), height()) - 2 * margin;
    if (side <= 0)
        return;

    // Ganzzahliger Faktor, damit die Module scharf bleiben.
    const int scale = qMax(1, side / qrImage.width());
    const QImage scaled = qrImage.scaled(qrImage.width() * scale, qrImage.height() * scale,
                                         Qt::IgnoreAspectRatio, Qt::FastTransformation);
    const QPoint topLeft = rect().center() - QPoint(scaled.width() / 2, scaled.height() / 2);
    p.drawImage(topLeft, scaled);
}
