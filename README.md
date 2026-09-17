# 📸 Fotobox (Qt6)

Eine private Fotobox-Anwendung auf Basis von **Qt6** für **Raspberry Pi** und **Linux-Hosts**.

Gedacht für Events wie:
- Geburtstage
- Partys
- Hochzeiten

Die Fotobox kombiniert:
- 📷 **Livebild** über eine Raspberry-Pi-Kamera (CSI)
- 📸 **Fotoaufnahme** über eine zweite Kamera (DSLR via `gphoto2`)
- 🖐 **Touchscreen-Bedienung**
- 🔘 **Physischen Auslöse-Button**
- 🖼 **Slideshow im Idle-Zustand**
- ⏱ **Countdown + Overlay**
- ☁️ **Nextcloud-Anbindung inkl. QR-Code**

> Kein Dauerbetrieb.  
> Kein Produkt.  
> Ein solides, sauberes DIY-Projekt.

---

## ✨ Features

- Permanentes **Livebild** (Raspberry Pi Kamera)
- **Slideshow** im Idle-Zustand
- **Touch auf dem Display**
  - beendet die Slideshow
  - löst **kein** Foto aus
- **Physischer Button**
  - startet Countdown (3…2…1)
  - löst ein Foto über die DSLR aus
- Anzeige des Fotos nach der Aufnahme
- Danach **3 Minuten Livebild**
- Automatischer Rücksprung zur Slideshow
- QR-Code für Nextcloud-Galerie
- Saubere Trennung von UI, Hardware und Logik

---

## 🧱 Architektur (Kurzfassung)

### 📷 Kameras

**LiveViewCamera**
- Raspberry Pi Kamera (CSI)
- liefert kontinuierliche Frames
- läuft **immer**

**Camera**
- DSLR über `gphoto2`
- macht **genau ein Foto**

---

### 🖐 Eingaben

**TouchInput**
- erkennt Touch auf dem Display
- beendet nur die Slideshow

**TriggerThread**
- physischer GPIO-Button
- startet Countdown + Foto

---

### 🖼 UI-Konzept

- Livebild = **Basis-Ebene**
- Slideshow, Countdown und Foto = **Overlays**
- Livekamera wird **nie gestoppt**

---

## 📂 Projektstruktur

```
src/
├── core/                    # Konfiguration, Dateiwatcher
│   ├── Config.*             # JSON-Konfiguration mit Standardwerten
│   └── FileWatcher.*        # beobachtet den Bildordner
│
├── hardware/                # Kameras & Auslöser
│   ├── Camera.*             # DSLR-Interface (asynchron)
│   ├── CameraDummy.*        # Testbild
│   ├── CameraGPhoto.*       # gphoto2 als Prozess
│   ├── LiveViewCamera.*     # Livebild-Interface
│   ├── LiveViewCameraDummy.*
│   ├── LiveViewCameraRpicam.*  # rpicam-vid MJPEG-Stream (Pi)
│   ├── LiveViewCameraQt.*   # Webcam über Qt Multimedia (Host)
│   ├── CameraSnapshot.*     # Foto = aktueller Livebild-Frame
│   ├── LedController.*      # LED-Helligkeit (Dummy)
│   ├── LedPwm.*             # LED über PWM am Pi (Platzhalter)
│   ├── Trigger.*            # Auslöser-Interface
│   ├── TriggerKeyboard.*    # Leertaste / Enter
│   ├── TriggerTimer.*       # automatisch, für Tests
│   ├── TriggerGpio.*        # Button am Pi (Platzhalter)
│   └── HardwareFactory.*    # wählt Backends per Konfiguration
│
├── input/                   # Eingaben
│   └── TouchInput*.*        # Touch / Mausklick mit Debounce
│
├── ui/                      # UI-Komponenten
│   ├── Theme.*              # Designs (Farben, Schriften, Stylesheets)
│   ├── QrCard.*             # QR-Code, Link, Passwort
│   ├── SettingsPanel.*      # Einstellungs-Overlay
│   ├── QrCodeWidget.*
│   └── qrcodegen.*
│
├── MainWindow.*             # Zustandsmaschine + Overlays
└── main.cpp                 # Kommandozeile, Start
```

---

## ▶️ Starten

```bash
./build/fotobox                 # Vollbild
./build/fotobox --windowed      # im Fenster (Entwicklung)
./build/fotobox --help          # alle Optionen
```

Nützliche Optionen:

| Option | Bedeutung |
|---|---|
| `-c, --config <datei>` | Pfad zur `config.json` |
| `-w, --windowed` | Fenster statt Vollbild |
| `--camera dummy\|gphoto` | Foto-Backend erzwingen |
| `--liveview dummy\|rpicam` | Livebild-Backend erzwingen |
| `--trigger keyboard\|gpio\|timer` | Auslöser erzwingen |
| `--theme <id>` | Design erzwingen (retro, neon, elegant, wedding-blush, wedding-green, pool) |
| `--screenshot <datei> --show settings` | Bild rendern und beenden (für Tests) |

Tasten: **Leertaste / Enter** lösen ein Foto aus, **Esc / Q** beenden.
Das Zahnrad oben links öffnet die Einstellungen: Design, LED-Helligkeit, Countdown, Anzeigedauer, QR-Code.

### Designs

Sechs Designs sind eingebaut und im Einstellungsmenü umschaltbar: **Retro Photobooth** (Standard),
**Neon Party**, **Elegant**, **Hochzeit Rosé**, **Hochzeit Botanik** und **Pool Party**.
Sie liegen als Token-Sets in `src/ui/Theme.cpp`.
Die vorgesehenen Schriften (Archivo Black, Special Elite, Bebas Neue, Rubik, Cormorant Garamond, Lato,
Great Vibes, Montserrat, Playfair Display, Fredoka, Nunito)
werden aus einem Ordner `fonts/` neben dem Programm oder im Arbeitsverzeichnis geladen, wenn vorhanden.
Fehlen sie, greifen Systemschriften.

Ohne Angabe wird die Konfiguration in dieser Reihenfolge gesucht:
`$FOTOBOX_CONFIG`, `./config.json`, `~/.config/fotobox/config.json`.
Fehlt sie, laufen Standardwerte mit dem Bildordner `~/Pictures/Fotobox/<Jahr>/Test_Event`.

---

## 🛠️ Build (Host)

Getestet unter **Ubuntu 24.xx** und **macOS** (Qt 6.11 aus dem Qt-Installer).
Details in [BUILD.md](BUILD.md).

### Abhängigkeiten

```bash
sudo apt install \
  build-essential \
  cmake \
  ninja-build \
  qt6-base-dev \
  qt6-base-dev-tools
```

### Build & Start

```bash
cmake -B build -G Ninja
ninja -C build
./build/fotobox
```

---

## 🛠️ Build (Raspberry Pi)

Zusätzlich benötigt:

- `gphoto2`
- Raspberry Pi Kamera  
  (`libcamera` / `Picamera2`, je nach Implementierung)

```bash
sudo apt install gphoto2
```

Build erfolgt identisch, optional mit:

```bash
-DFOTOBOX_PI=ON
```

Damit werden am Pi standardmäßig `gphoto`, `rpicam` und `gpio` als Backends gewählt.

---

## ⚙️ Konfiguration

Die Anwendung nutzt eine **lokale JSON-Konfigurationsdatei**,  
die bewusst **nicht** im Repository liegt.

### Beispiel: `Config.example.json`

Alle Felder sind optional, siehe [Config.example.json](Config.example.json).

| Abschnitt | Feld | Bedeutung |
|---|---|---|
| SystemData | basePath, yearPath, partyPath | Bildordner = basePath/yearPath/partyPath (`~` erlaubt) |
| SystemData | link | URL für den QR-Code |
| Behaviour | countdownSeconds | Countdown-Länge |
| Behaviour | slideshowIntervalMs, photoDisplayMs, liveViewTimeoutMs | Zeiten in ms |
| Behaviour | showQr, qrSize | QR-Code ein/aus und Kantenlänge |
| Behaviour | theme | Design-ID, siehe Abschnitt Designs |
| Hardware | led, ledBrightness | LED-Backend und Helligkeit in Prozent |
| Hardware | camera, liveView, trigger | `auto` oder ein konkretes Backend (`camera`: dummy, snapshot, gphoto; `liveView`: dummy, qt, rpicam) |
| Hardware | gphoto2Binary, rpicamBinary | Programmnamen/Pfade |
| Hardware | liveViewWidth/Height/Fps | Auflösung des Livebilds |
| Hardware | gpioChip, gpioLine | Button am Pi |

> Die echte `config.json` wird per `.gitignore` ausgeschlossen.

---

## 📸 Ablauf (User-Sicht)

1. Slideshow läuft
2. Touch → Slideshow endet
3. Button → Countdown **3…2…1**
4. Foto wird aufgenommen
5. Foto kurz angezeigt
6. Livebild läuft **3 Minuten**
7. Keine Aktion → Slideshow startet erneut

---

## ⚠️ Hinweise

- Nicht für Dauerbetrieb gedacht
- Keine Garantie
- Keine Sicherheitsfeatures
- Keine Cloud-Sync-Fehlerbehandlung auf Enterprise-Niveau

**Aber:**
- ✔ stabil genug für eine Party
- ✔ überschaubar genug, um es zu warten

---

## 📜 Lizenz

Privates Projekt.  
Nutzung auf eigene Gefahr.

Der Code darf:
- gelesen
- verstanden
- angepasst

werden.

---

## ❤️ Motivation

Gebaut, weil:

- fertige Fotoboxen teuer sind
- DIY mehr Spaß macht
- man irgendwann keine Lust mehr auf schwarze Bildschirme hatte
