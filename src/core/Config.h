#pragma once
#include <QString>

// Zentrale Konfiguration der Fotobox.
// Wird aus einer JSON-Datei geladen, alle Felder sind optional
// und haben sinnvolle Standardwerte.
struct Config
{
    // --- Pfade / Event (Abschnitt "SystemData") ---
    QString basePath;     // z.B. /home/fotobox/Nextcloud/  ("~" wird expandiert)
    QString yearPath;     // z.B. 2026
    QString partyPath;    // z.B. Mein_Event
    QString partyname;    // Anzeigename
    QString link;         // URL, die als QR-Code angezeigt wird
    QString password;     // optional, z.B. fuer den Share-Link

    // --- Verhalten (Abschnitt "Behaviour") ---
    int  countdownSeconds   = 3;
    int  slideshowIntervalMs = 4000;
    int  photoDisplayMs     = 3000;
    int  liveViewTimeoutMs  = 3 * 60 * 1000;
    bool showQr             = true;
    int  qrSize             = 130;
    QString theme           = "retro";  // retro | neon | elegant

    // --- Hardware (Abschnitt "Hardware") ---
    // Backends: "auto" waehlt je nach Build (Pi oder Host).
    QString cameraBackend   = "auto";   // auto | dummy | snapshot | gphoto
    QString liveViewBackend = "auto";   // auto | dummy | qt | rpicam
    QString triggerBackend  = "auto";   // auto | keyboard | gpio | timer
    int     autoTriggerMs   = 10000;    // nur fuer trigger = timer
    QString gphoto2Binary   = "gphoto2";
    QString rpicamBinary    = "rpicam-vid";
    int     liveViewWidth   = 1280;
    int     liveViewHeight  = 720;
    int     liveViewFps     = 30;
    QString gpioChip        = "gpiochip0";
    int     gpioLine        = 17;
    QString ledBackend      = "auto";   // auto | dummy | pwm
    int     ledBrightness   = 50;       // 0..100 %
    int     ledPwmChip      = 0;
    int     ledPwmChannel   = 0;

    // Pfad der geladenen Datei (leer = Standardwerte)
    QString sourcePath;

    // Vollstaendiger Bildordner: basePath/yearPath/partyPath
    QString fullImagePath() const;

    static Config defaults();
    static Config load(const QString& path);

    // Schreibt die aenderbaren Felder zurueck in die Datei (sourcePath,
    // sonst ./config.json). Unbekannte Felder in der Datei bleiben erhalten.
    bool save();

    // Sucht die Konfiguration in der ueblichen Reihenfolge:
    // $FOTOBOX_CONFIG, ./config.json, ~/.config/fotobox/config.json
    static QString findDefaultPath();
};
