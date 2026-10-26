# Schriften

Diese Dateien werden beim Start geladen, damit die Designs überall gleich aussehen.
Ohne sie greift die Anwendung auf Systemschriften zurück, das Ergebnis weicht dann ab.

Gesucht wird in dieser Reihenfolge:

1. `fonts/` neben dem Programm
2. `/usr/share/fotobox/fonts` (so installiert das Paket sie)
3. `fonts/` im Arbeitsverzeichnis
4. `fonts/` neben der Konfigurationsdatei

| Design | Überschriften | Fließtext |
|---|---|---|
| Retro Photobooth | Archivo Black | Special Elite |
| Neon Party | Bebas Neue | Rubik |
| Elegant | Cormorant Garamond | Lato |
| Hochzeit Rosé | Great Vibes | Montserrat |
| Hochzeit Botanik | Playfair Display | Lato |
| Pool Party | Fredoka | Nunito |

## Herkunft und Lizenzen

Alle Dateien stammen unverändert aus <https://github.com/google/fonts>.

- **Special Elite**: Apache License 2.0
- **alle übrigen**: SIL Open Font License 1.1

Die vollständigen Lizenztexte liegen in [licenses/](licenses/). Dateien mit
`-Variable` im Namen sind variable Schriften; umbenannt wurde nur der Dateiname,
der Inhalt ist unverändert.
