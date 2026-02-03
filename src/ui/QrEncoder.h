#pragma once
#include <QImage>
#include <QString>
#include "qrcodegen.hpp"

class QrEncoderQt
{
public:
    static QImage encode(const QString& text, int scale = 8)
    {
        if (text.isEmpty()) {
            QImage img(200, 200, QImage::Format_RGB32);
            img.fill(Qt::lightGray);
            return img;
        }

        using qrcodegen::QrCode;

        QrCode qr = QrCode::encodeText(
            text.toUtf8().constData(),
            QrCode::Ecc::LOW
        );

        const int size = qr.getSize();
        const int imgSize = size * scale;

        QImage img(imgSize, imgSize, QImage::Format_RGB32);
        img.fill(Qt::white);

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                if (qr.getModule(x, y)) {
                    for (int ys = 0; ys < scale; ++ys) {
                        for (int xs = 0; xs < scale; ++xs) {
                            img.setPixelColor(
                                x * scale + xs,
                                y * scale + ys,
                                Qt::black
                            );
                        }
                    }
                }
            }
        }

        return img;
    }
};
