#pragma once
#include <QString>

class Camera {
public:
    virtual ~Camera() = default;
    virtual QString capture(const QString& targetDir) = 0;
};

Camera* createCamera();