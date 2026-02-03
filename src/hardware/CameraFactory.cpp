#include "LiveViewCamera.h"

#ifdef FOTOBOX_PI
// später echte Raspi-Implementierung
#include "LiveViewCameraRaspi.h"
#else
#include "LiveViewCameraDummy.h"
#endif

LiveViewCamera* createLiveViewCamera()
{
#ifdef FOTOBOX_PI
    return new LiveViewCameraRaspi();
#else
    return new LiveViewCameraDummy();
#endif
}
