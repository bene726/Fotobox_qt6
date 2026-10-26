#pragma once

// Diagnose: zeigt die verfuegbaren Videoquellen (USB-Webcams).
//
//   fotobox --list-cameras
//
// Laeuft ohne Bildschirm, also auch ueber SSH.
namespace CameraScan {
int run();
}
