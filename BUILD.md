# Build Guide – Fotobox

## Übersicht
Diese Datei beschreibt den Build der Fotobox für:
- Linux Host (Entwicklung)
- Raspberry Pi (Zielsystem)

---

## Build auf Linux Host (Ubuntu 24.xx)

### Abhängigkeiten
```bash
sudo apt install \
  build-essential \
  cmake \
  ninja-build \
  qt6-base-dev \
  qt6-base-dev-tools
```

### Build
```bash
cmake -B build -G Ninja
ninja -C build
./build/fotobox
```

### Typische Fehler
- **Qt Headers fehlen** → `qt6-base-dev` prüfen
- **Linkerfehler bei QObject** → `AUTOMOC` aktivieren
- **Pfadprobleme** → `Config.js` prüfen

---

## Build auf Raspberry Pi

### Zusätzliche Abhängigkeiten
```bash
sudo apt install gphoto2
```

Für Livebild:
- libcamera / Picamera2 (abhängig von Implementierung)

### Build
```bash
cmake -B build -G Ninja -DFOTOBOX_PI=ON
ninja -C build
```

---

## Debug-Tipps
- `qDebug()` für Pfade und Zustände nutzen
- Build immer **clean**, wenn neue QObject-Klassen dazukommen:
```bash
rm -rf build
```
