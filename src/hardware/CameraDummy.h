#pragma once
#include "Camera.h"

class CameraDummy : public Camera
{
public:
    QString capture(const QString& targetDir) override;
};
