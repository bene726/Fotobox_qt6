#pragma once
#include <QObject>

struct Config;
class Camera;
class LiveViewCamera;
class Trigger;
class LedController;

// Waehlt die passenden Hardware-Backends anhand der Konfiguration.
// "auto" bedeutet: am Pi echte Hardware, am Host Webcam/Dummies/Tastatur.
LiveViewCamera* createLiveViewCamera(const Config& cfg, QObject* parent);
Camera*         createCamera(const Config& cfg, LiveViewCamera* live, QObject* parent);
Trigger*        createTrigger(const Config& cfg, QObject* parent);
LedController*  createLed(const Config& cfg, QObject* parent);
