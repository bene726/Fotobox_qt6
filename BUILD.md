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
- **Pfadprobleme** → `config.json` prüfen oder mit `--config` angeben

---

## Build auf macOS (Qt aus dem Qt-Installer)

Qt 6.11, CMake und Ninja liegen unter `~/Qt`. Einmalig in die Shell aufnehmen:
```bash
export PATH="$HOME/Qt/6.11.2/macos/bin:$HOME/Qt/Tools/CMake/CMake.app/Contents/bin:$HOME/Qt/Tools/Ninja:$PATH"
```

Für Tests mit der DSLR am Mac:
```bash
brew install gphoto2
```

### Build
```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="$HOME/Qt/6.11.2/macos"
ninja -C build
./build/fotobox --windowed
```

Alternativ das Projekt in Qt Creator über `CMakeLists.txt` öffnen.

---

## Build auf Raspberry Pi

### Zusätzliche Abhängigkeiten
```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools \
  gphoto2 libgpiod-dev gpiod rpicam-apps
```

- `rpicam-apps` liefert `rpicam-vid` für das Livebild (MJPEG-Stream)
- `libgpiod` für den Button (Pi 5 hat kein sysfs-GPIO mehr)

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
