#include "Camera.h"
#include <QImage>
#include <QDateTime>

class CameraDummy : public Camera {
public:
    QString capture(const QString& dir) override {
        QString file = dir + "/dummy_" +
            QDateTime::currentDateTime().toString("hhmmss") + ".jpg";
        QImage img(800, 600, QImage::Format_RGB32);
        img.fill(Qt::darkGray);
        img.save(file);
        return file;
    }
};
