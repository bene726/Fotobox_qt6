#pragma once
#include "Camera.h"

// Erzeugt ein Testbild mit Zeitstempel, ohne echte Hardware.
class CameraDummy : public Camera
{
    Q_OBJECT
public:
    using Camera::Camera;
    void capture(const QString& targetDir) override;
};
