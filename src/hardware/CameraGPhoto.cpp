#ifdef FOTOBOX_PI
#include "Camera.h"
#include <QProcess>
#include <QDateTime>

class CameraGPhoto : public Camera {
public:
    QString capture(const QString& dir) override {
        QString file = dir + "/photo_" +
            QDateTime::currentDateTime().toString("hhmmss") + ".jpg";

        QProcess::execute("gphoto2",
            {"--capture-image-and-download", "--filename", file});
        return file;
    }
};

Camera* createCamera() { return new CameraGPhoto(); }
#endif
