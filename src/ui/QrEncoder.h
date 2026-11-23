#pragma once
#include <QDebug>
#include <QImage>
#include <QString>
#include "qrcodegen.hpp"

class QrEncoderQt
{
public:
    static QImage encode(const QString& text, int scale = 8)
    {
        if (text.isEmpty())
            return placeholder();

        using qrcodegen::QrCode;

        // Ein QR-Code traegt hoechstens 2953 Bytes. Der Link kommt aus der
        // Nextcloud-Datei und wird vom Handy aus getippt; passt er nicht, wirft
        // qrcodegen eine Ausnahme. Unbehandelt beendete sie die Fotobox, und
        // zwar aus dem Zeichnen heraus - also hier abfangen.
        try {
            return render(QrCode::encodeText(text.toUtf8().constData(), QrCode::Ecc::LOW),
                          scale);
        } catch (const std::exception& e) {
            qWarning() << "QR-Code nicht erzeugbar, Link zu lang?" << e.what();
            return placeholder();
        }
    }

private:
    static QImage render(const qrcodegen::QrCode& qr, int scale)
    {
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

    // Graues Feld anstelle des Codes: ohne Link oder bei einem Fehler soll
    // etwas Sichtbares stehen, kein leeres Bild.
    static QImage placeholder()
    {
        QImage img(200, 200, QImage::Format_RGB32);
        img.fill(Qt::lightGray);
        return img;
    }
};
