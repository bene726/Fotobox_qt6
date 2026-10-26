# 📸 Fotobox (Qt6)

[![Build & Publish](https://github.com/bene726/Fotobox_qt6/actions/workflows/build.yml/badge.svg)](https://github.com/bene726/Fotobox_qt6/actions/workflows/build.yml)

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

- Permanentes **Livebild** (USB-Webcam oder Raspberry-Pi-Kamera)
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

Zwei getrennte Kameras mit klaren Aufgaben.

**LiveViewCamera** – liefert dauerhaft Frames, läuft **immer**:

| Backend | Quelle | Wann |
|---|---|---|
| `qt` | USB-Webcam über V4L2 | Standard für eine Webcam, auch am Mac |
| `rpicam` | Raspberry-Pi-Kamera am CSI-Anschluss | wenn eine Pi-Kamera verbaut ist |
| `dummy` | Testbild | ohne Hardware |

Welche Webcams erkannt werden, zeigt `fotobox --list-cameras`. Sind mehrere
angeschlossen, wählt `"liveViewDevice"` die richtige über einen Teil des Namens.

**Camera** – macht **genau ein Foto** pro Auslösung:

| Backend | Quelle |
|---|---|
| `gphoto` | Spiegelreflex über `gphoto2`, z. B. Canon EOS 600D |
| `snapshot` | aktuelles Livebild als Foto, ohne DSLR |
| `dummy` | Testbild |

> Eine Spiegelreflex im PTP-Modus ist **keine** Videoquelle. Sie taucht deshalb
> nicht bei `--list-cameras` auf, sondern wird nur über `gphoto2` angesprochen.

#### Spiegelreflex am Raspberry Pi

Auf einem Desktop-System greift `gvfs` ungefragt auf angeschlossene Kameras zu
und blockiert damit `gphoto2`. Der Fehler lautet dann „Konnte das USB-Gerät nicht
beanspruchen" bzw. `Could not claim the USB device` (Code −53). Dauerhaft abstellen:

```bash
systemctl --user mask gvfs-gphoto2-volume-monitor.service && systemctl --user stop gvfs-gphoto2-volume-monitor.service && pkill -f gvfs-gphoto2
```

Die Fotobox warnt beim Start, wenn `gvfs-gphoto2` läuft, und zeigt im Fehlerfall
„Kamera ist von einem anderen Programm belegt" statt der technischen Meldung.

Verbindung testen:

```bash
gphoto2 --auto-detect
gphoto2 --capture-image-and-download --filename /tmp/test.jpg
```

---

### 🖐 Eingaben

**TouchInput**
- erkennt Touch auf dem Display
- beendet nur die Slideshow

**Trigger**
- physischer Taster am GPIO, siehe [GPIO.md](GPIO.md)
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
│   ├── LedPwm.*             # LED über Hardware-PWM am Pi
│   ├── Trigger.*            # Auslöser-Interface
│   ├── TriggerKeyboard.*    # Leertaste / Enter
│   ├── TriggerTimer.*       # automatisch, für Tests
│   ├── TriggerGpio.*        # Taster am Pi (Kernel-uAPI v2)
│   ├── GpioScan.*           # Diagnose: Pin-Belegung ermitteln
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
| `--list-cameras` | erkannte Webcams für das Livebild auflisten |
| `--gpio-info` | GPIO-Leitungen auflisten (Pi) |
| `--gpio-watch` | Taster-Pin ermitteln: drücken und ablesen (Pi) |

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

### Start auf dem Raspberry Pi

Die Fotobox ist eine Anwendung mit Oberfläche und braucht einen Bildschirm.

| Situation | Befehl |
|---|---|
| Auf dem Desktop des Pi | `fotobox` |
| Über SSH | nur die Diagnose-Optionen, siehe Tabelle oben |
| Ohne Desktop, direkt auf dem Display | `QT_QPA_PLATFORM=eglfs fotobox` |

Über SSH gibt es keine Anzeige. Startet man dort die Oberfläche, meldet Qt
`could not connect to display`. Die Optionen `--list-cameras`, `--gpio-info`,
`--gpio-watch`, `--help` und `--version` laufen dagegen auch über SSH.

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

## 📦 Installation auf dem Raspberry Pi (apt)

Fertige Pakete für **Raspberry Pi OS 13 (Trixie), 64-bit** liegen in einem
Apt-Repository auf GitHub Pages: <https://bene726.github.io/Fotobox_qt6/>

```bash
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://bene726.github.io/Fotobox_qt6/fotobox.gpg | sudo tee /etc/apt/keyrings/fotobox.gpg >/dev/null
. /etc/os-release
echo "deb [arch=arm64 signed-by=/etc/apt/keyrings/fotobox.gpg] https://bene726.github.io/Fotobox_qt6 $VERSION_CODENAME main" \
  | sudo tee /etc/apt/sources.list.d/fotobox.list
sudo apt update && sudo apt install fotobox
```

Danach kommen Updates ganz normal über `sudo apt upgrade`.

- **Komponente `main`**: Releases (Git-Tags `v*`).
- **Komponente `nightly`**: jeder Stand von `main`. Dafür `main nightly` in die Zeile schreiben.
- Ist das Repo (noch) nicht signiert, fehlt `fotobox.gpg`; dann statt `signed-by=...` die Option `trusted=yes` verwenden.

Das Paket installiert `fotobox` nach `/usr/bin`, eine Desktop-Datei und `Config.example.json`
unter `/usr/share/doc/fotobox/`. Die eigene Konfiguration gehört nach `~/.config/fotobox/config.json`.

### Pipeline

[`.github/workflows/build.yml`](.github/workflows/build.yml) baut bei jedem Push auf `main` und bei Tags
die `.deb`-Pakete in Debian-Containern auf einem ARM64-Runner, legt ein GitHub-Release an
(rollendes Pre-Release `nightly` bzw. das Release zum Tag) und pflegt das Apt-Repo im Branch `gh-pages`,
das per GitHub Pages veröffentlicht wird. Das Repo wird mit [`scripts/apt_repo.py`](scripts/apt_repo.py) erzeugt,
die letzten fünf Nightlies bleiben erhalten.

Ein Release entsteht durch einen Tag:

```bash
git tag -a v0.1.0 -m "Erstes Release" && git push origin v0.1.0
```

Der Tag muss dem Schema `vMAJOR.MINOR.PATCH` folgen, sonst bricht die Pipeline mit einer
klaren Meldung ab. Ein Tag mit Suffix wie `v0.2.0-rc1` wird automatisch als Vorabversion
markiert und von `gh` nicht als „latest“ geführt.

Der Release-Text wird von [`scripts/release_notes.py`](scripts/release_notes.py) erzeugt und enthält:

- alle Commits seit dem vorherigen Release-Tag, jeweils verlinkt
- einen Vergleichslink auf die Änderungen zwischen beiden Tags
- die fertigen Installationsbefehle für den Pi
- eine Tabelle der Pakete samt SHA256-Prüfsummen (auch als `SHA256SUMS` am Release)

Nach dem Erzeugen prüft die Pipeline, dass die neue Version wirklich in der passenden
Apt-Komponente steht und die Signaturen gültig sind. Andernfalls schlägt der Lauf fehl,
statt ein kaputtes Repository zu veröffentlichen.

### Einmalige Einstellung für Tag-Läufe

Das Veröffentlichen auf GitHub Pages läuft über die Umgebung `github-pages`. Deren
Schutzregeln erlauben standardmäßig nur den Standard-Branch, ein Tag-Lauf wird dadurch
abgewiesen. Damit Releases die Seite aktualisieren dürfen, einmalig unter
**Settings → Environments → github-pages → Deployment branches and tags** eine Regel für
`v*` hinzufügen.

Release und Apt-Repo liegen bewusst in einem eigenen Job ohne diese Umgebung. Selbst wenn
das Deployment abgewiesen wird, entstehen Release und Pakete trotzdem, nur die Seite bleibt
dann auf dem vorherigen Stand.

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
| Hardware | camera, liveView, trigger, led | `auto` oder ein konkretes Backend, siehe Abschnitt Kameras |
| Hardware | liveViewDevice | Teil des Kameranamens, wenn mehrere Webcams angeschlossen sind |
| Hardware | gphoto2Binary, rpicamBinary | Programmnamen/Pfade |
| Hardware | liveViewWidth/Height/Fps | Auflösung des Livebilds |
| Hardware | gpioChip, gpioLine, gpioPull, gpioDebounceMs | Taster am Pi, Details in [GPIO.md](GPIO.md) |
| Hardware | ledPwmChip, ledPwmChannel, ledPwmFrequency | Hardware-PWM für die LED-Beleuchtung |

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
