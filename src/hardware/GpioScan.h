#pragma once

// Diagnosewerkzeuge, um die Belegung des GPIO-Headers zu ermitteln.
//
//   fotobox --gpio-info    zeigt alle Leitungen mit Name und Belegung
//   fotobox --gpio-watch   zeigt zusaetzlich, welche Leitung sich aendert
//
// Gedacht fuer den Fall, dass die Pin-Nummern der vorhandenen Verdrahtung
// nicht bekannt sind: Taster druecken, Nummer ablesen.
namespace GpioScan {
int run(bool watch);
}
