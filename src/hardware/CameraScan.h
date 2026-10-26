#pragma once

// Diagnose: zeigt die verfuegbaren Videoquellen (USB-Webcams).
//
//   fotobox --list-cameras
//
// Laeuft ohne Bildschirm, also auch ueber SSH.
#include <QList>
#include <QPair>
#include <QString>

struct Config;

namespace CameraScan {
int run();

// Oeffnet das konfigurierte Livebild-Backend und zaehlt fuer ein paar Sekunden
// die ankommenden Bilder. Beantwortet die Frage "kommt ueberhaupt etwas an?"
// ohne Bildschirm.
int test(const Config& cfg, int seconds = 5);

// Verfuegbare Videoquellen als (Kennung, Anzeigename).
// Leer, wenn ohne Qt Multimedia gebaut oder keine Kamera vorhanden ist.
QList<QPair<QString, QString>> devices();
}
