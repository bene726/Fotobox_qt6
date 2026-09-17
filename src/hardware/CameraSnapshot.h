#pragma once
#include "Camera.h"

class LiveViewCamera;

// "Foto" = aktueller Frame des Livebilds. Braucht keine DSLR.
// Praktisch fuer Tests am Host und als Notloesung am Pi.
class CameraSnapshot : public Camera
{
    Q_OBJECT
public:
    explicit CameraSnapshot(LiveViewCamera* live, QObject* parent = nullptr);
    void capture(const QString& targetDir) override;

private:
    LiveViewCamera* m_live;
};
