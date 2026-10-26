#pragma once

// Diagnose: zeigt die verfuegbaren Videoquellen (USB-Webcams).
//
//   fotobox --list-cameras
//
// Laeuft ohne Bildschirm, also auch ueber SSH.
#include <QList>
#include <QPair>
#include <QString>

namespace CameraScan {
int run();

// Verfuegbare Videoquellen als (Kennung, Anzeigename).
// Leer, wenn ohne Qt Multimedia gebaut oder keine Kamera vorhanden ist.
QList<QPair<QString, QString>> devices();
}
