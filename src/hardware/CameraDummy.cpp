#include "CameraDummy.h"

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QTimer>

void CameraDummy::capture(const QString& targetDir)
{
    QDir().mkpath(targetDir);

    const QDateTime now = QDateTime::currentDateTime();
    const QString file = targetDir + "/dummy_" + now.toString("yyyyMMdd_hhmmss") + ".jpg";

    QImage img(1200, 800, QImage::Format_RGB32);
    QLinearGradient grad(0, 0, img.width(), img.height());
    grad.setColorAt(0.0, QColor(40, 60, 120));
    grad.setColorAt(1.0, QColor(120, 40, 90));

    QPainter p(&img);
    p.fillRect(img.rect(), grad);
    p.setPen(Qt::white);
    { QFont f; f.setPointSize(48); f.setBold(true); p.setFont(f); }
    p.drawText(img.rect(), Qt::AlignCenter,
               "Dummy Foto\n" + now.toString("hh:mm:ss"));
    p.end();

    if (!img.save(file, "JPG", 90)) {
        emit captureFailed("Testbild konnte nicht gespeichert werden: " + file);
        return;
    }

    // Simulierte Ausloeseverzoegerung einer echten Kamera.
    QTimer::singleShot(800, this, [this, file] { emit captured(file); });
}
