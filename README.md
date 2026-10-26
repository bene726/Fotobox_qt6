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
  - eigener Auslöse-Knopf auf dem Bildschirm für ein Foto
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
angeschlossen, wählt man die richtige im Einstellungsmenü oder über `"liveViewDevice"`.

Bleibt das Bild schwarz, beantwortet `fotobox --test-camera` die entscheidende Frage,
ob überhaupt Bilder ankommen:

```
Erstes Bild nach 199 ms, 1280x720
77 Bilder in 5 s (15.4 pro Sekunde). Das Livebild funktioniert.
```

Beide Befehle laufen auch über SSH, ohne Bildschirm.

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
systemctl --user mask gvfs-gphoto2-volume-monitor.service && systemctl --user stop gvfs-gphoto2-volume-monitor.service && pkill -f 'gvfsd?-gphoto2'
```

> Es gibt **zwei** Prozesse: den Monitor `gvfs-gphoto2-volume-monitor` und den
> Dienst `gvfsd-gphoto2`. Ein Muster ohne das `d` erwischt nur einen davon.

Hält die Kamera danach immer noch jemand fest, zeigt das folgender Befehl:

```bash
sudo fuser -v /dev/bus/usb/001/002   # Busnummern aus "gphoto2 --auto-detect"
```

Die Fotobox warnt beim Start, wenn gvfs läuft, und zeigt im Fehlerfall
„Kamera ist von einem anderen Programm belegt" statt der technischen Meldung.

Verbindung testen:

```bash
gphoto2 --auto-detect
gphoto2 --capture-image-and-download --filename /tmp/test.jpg
```

#### Wie lange eine Aufnahme dauert

Gemessen auf einem Pi 5 mit einer Canon EOS 600D:

| Schritt | Zeit |
|---|---|
| Kamera öffnen und abfragen | 0,1 s |
| Auslösen und Bild übertragen | 2,5 s |

Die Zeit geht fast vollständig für die Übertragung über USB 2.0 drauf. Der
wirksamste Hebel ist deshalb die Bildgröße in der Kamera:

```bash
gphoto2 --get-config imageformat          # aktuelle Einstellung
gphoto2 --set-config imageformat=2        # Medium Fine JPEG
```

Ein Objektiv auf manuellem Fokus spart zusätzlich die Zeit für den Autofokus
und kann bei wenig Licht nicht danebenliegen.

Die verbleibende Wartezeit überbrückt die Fotobox selbst: sobald der Countdown
abgelaufen ist, friert sie das Livebild ein, damit sofort etwas zu sehen ist.
Das echte Foto ersetzt es, sobald es da ist.

---

### 🖐 Eingaben

**TouchInput**
- erkennt Touch auf dem Display
- beendet die Slideshow; ein Tippen daneben löst **kein** Foto aus

**Auslöser auf dem Bildschirm**
- eigener Knopf unten, im Stil des gewählten Designs
- bewusst nur dieser Knopf, damit nicht jede Berührung ein Foto macht

**Trigger**
- physischer Taster am GPIO, siehe [GPIO.md](GPIO.md)
- startet Countdown + Foto

---

### 🔌 Vorgeschlagene Pinbelegung

Zwei Verbraucher: der Auslöse-Taster und das PWM-Signal für die LED-Beleuchtung.
Beide Vorschläge liegen **direkt neben einem GND-Pin in derselben Reihe**, es genügt
also je ein zweipoliger Stecker.

| Funktion | GPIO (BCM) | Header-Pin | GND daneben | Warum |
|---|---|---|---|---|
| Auslöse-Taster | **17** | 11 | Pin 9 | frei, keine Sonderfunktion, kein Bus |
| LED-Beleuchtung | **12** | 32 | Pin 30 | Hardware-PWM Kanal 0, beim Booten low |

> In der Konfiguration steht die **BCM-Nummer**, nicht die Pin-Nummer am Header.
> GPIO 17 ist Pin 11, nicht Pin 17.

Warum ausgerechnet GPIO 12 für die LED, obwohl auf dem Pi 5 jeder Pin ginge:

1. **Dunkel beim Booten.** Die Pins 0 bis 8 werden beim Hochfahren auf High gezogen.
   Hinge der Transistor dort, würde die Beleuchtung während des Bootens leuchten.
   GPIO 12 liegt im Bereich, der auf Low gezogen wird.
2. **Funktioniert auch auf einem Pi 4.** GPIO 12 ist dort einer der vier Pins mit
   echter Hardware-PWM. Dieselbe Verdrahtung läuft also auf beiden Modellen, nur die
   Zeile in der `config.txt` unterscheidet sich.
3. **GND liegt daneben.** Pin 30 und Pin 32 sind benachbart in derselben Reihe.
4. **Weit weg vom Taster.** Pin 11 und Pin 32 liegen an entgegengesetzten Enden,
   die Kabel kommen sich nicht ins Gehege.

Passend dazu in die `config.json`:

```json
"Hardware": {
  "gpioLine": 17,
  "gpioPull": "up",
  "ledPwmChannel": 0,
  "ledPwmFrequency": 1000
}
```

Und ans Ende von `/boot/firmware/config.txt`, danach neu starten:

```
[all]
dtoverlay=pwm-pio,gpio=12
```

Der Abschnitt `[all]` davor stellt sicher, dass die Zeile nicht in einem
modellspezifischen Block landet, falls die Datei welche enthält.

#### Welches Overlay auf welchem Modell

| Overlay | Läuft auf | Erlaubte Pins | Erzeugt von |
|---|---|---|---|
| `pwm-pio,gpio=N` | **nur Pi 5** | jedem GPIO 0–27 | PIO-Einheit des RP1, hardware-getaktet |
| `pwm,pin=N,func=F` | Pi 4 und älter | 12 (func=4), 13 (func=4), 18 (func=2), 19 (func=2) | PWM-Block des SoC |
| `pwm-gpio,gpio=N` | alle | jedem GPIO | Software-PWM im Kernel, am ungenauesten |

Auf dem **Pi 5** ist `pwm-pio` die erste Wahl: hardware-getaktet wie echte PWM, aber
ohne Einschränkung bei der Pinwahl. Die LED darf dort also an jedem freien GPIO
hängen, Umstecken ist nicht nötig.

Welche Kombinationen aus Pin und Funktion das installierte System kennt, verrät:

```bash
dtoverlay -h pwm-pio
dtoverlay -h pwm
```

#### Danach prüfen

```bash
ls -l /sys/class/pwm/        # muss mindestens einen pwmchip zeigen
cat /sys/class/pwm/pwmchip0/npwm
```

> Auf dem Pi 5 kann es **mehrere** PWM-Chips geben, etwa für den Lüfter. Die Fotobox
> protokolliert beim Start alle gefundenen und welchen sie nimmt. Greift sie daneben,
> legt `"ledPwmChip"` den richtigen fest.

#### Alternativen für den Taster

| Statt | Geht auch |
|---|---|
| GPIO 17 (Pin 11, GND Pin 9) | GPIO 26 (Pin 37, GND Pin 39) |
| | GPIO 23 (Pin 16, GND Pin 14) |
| | GPIO 24 (Pin 18, GND Pin 20) |

#### Diese Pins besser meiden

| Pins | Grund |
|---|---|
| GPIO 0, 1 | für das EEPROM von Aufsteckplatinen reserviert |
| GPIO 2, 3 | I2C mit fest verbauten Pull-ups; GPIO 3 weckt den Pi zusätzlich aus dem Halt |
| GPIO 14, 15 | serielle Konsole |
| GPIO 7 bis 11 | SPI0 |
| GPIO 0 bis 8 | beim Booten auf High, für die LED ungeeignet |

Ist die vorhandene Verdrahtung unbekannt, nennt `fotobox --gpio-watch` die Nummer
auf Knopfdruck. Details in [GPIO.md](GPIO.md).

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
| `--test-camera` | fünf Sekunden lang zählen, ob wirklich Bilder ankommen |
| `--gpio-info` | GPIO-Leitungen auflisten (Pi) |
| `--gpio-watch` | Taster-Pin ermitteln: drücken und ablesen (Pi) |

Tasten: **Leertaste / Enter** lösen ein Foto aus, **Esc / Q** beenden.
Das Zahnrad oben links öffnet die Einstellungen: Design, **Kamera**, LED-Helligkeit,
Countdown, Anzeigedauer und QR-Code. Die Kamera-Auswahl listet die erkannten Webcams
und schaltet sofort um, der Umweg über die Konfigurationsdatei entfällt also.

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

Wo die Konfiguration liegt, steht im Abschnitt [Konfiguration](#️-konfiguration).

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

Die Anwendung liest **mehrere JSON-Dateien übereinander**. Jede Ebene überschreibt
nur die Felder, die sie selbst enthält. Keine davon liegt im Repository.

| # | Datei | Gedacht für | Wer ändert sie |
|---|---|---|---|
| 1 | `/etc/fotobox/config.json` | Hardware, Pfade, Backends | einmal beim Einrichten, per SSH |
| 2 | `~/.config/fotobox/config.json` | was am Gerät eingestellt wird | die Fotobox selbst beim Speichern |
| 3 | Datei aus `--config` oder `$FOTOBOX_CONFIG` | Sonderfälle und Tests | von Hand |
| 4 | `<basePath>/fotobox.json` | **Link, Passwort, Eventname** | du, vom Handy aus über Nextcloud |

Die letzte Ebene liegt im Nextcloud-Ordner und wird dorthin synchronisiert. Damit
änderst du den QR-Link und das Passwort für das nächste Fest, ohne den Pi anzufassen.
Beim Start steht im Log, welche Dateien tatsächlich gelesen wurden.

### Einrichten auf dem Pi

```bash
sudo mkdir -p /etc/fotobox && sudo cp /usr/share/doc/fotobox/Config.example.json /etc/fotobox/config.json && sudo nano /etc/fotobox/config.json
```

### Die Datei in der Nextcloud

Sie liegt direkt in `basePath`, heißt standardmäßig `fotobox.json` und enthält nur
das, was sich pro Fest ändert:

```json
{
  "SystemData": {
    "link": "https://cloud.example/s/abc123",
    "password": "sommer2026",
    "partyname": "Sommerfest 2026",
    "partyPath": "Sommerfest"
  }
}
```

Ein anderer Dateiname geht über `"eventConfigFile"`, ein leerer Wert schaltet diese
Ebene ab.

> `basePath` wird in dieser Datei **ignoriert**. Sie liegt selbst darin und würde sich
> sonst den Boden unter den Füßen wegziehen. Die Fotobox schreibt dazu eine Warnung ins Log.

### Was wohin gehört

- **Hardware und Pfade** nach `/etc`: GPIO-Pins, PWM, Kamera-Backends, `basePath`.
- **Link, Passwort, Eventname** in die Nextcloud-Datei.
- **Design, LED-Helligkeit, Countdown** stellst du am Touchscreen ein, die Fotobox
  speichert sie selbst in Ebene 2.

> Steht ein Feld in der Nextcloud-Datei, gewinnt es immer. Eine Einstellung, die du am
> Touchscreen änderst, wird dadurch beim nächsten Start wieder überschrieben. Deshalb
> gehört dort nur hinein, was sich wirklich pro Fest ändert.

### Alle Felder: `Config.example.json`

Alle Felder sind optional, siehe [Config.example.json](Config.example.json).

| Abschnitt | Feld | Bedeutung |
|---|---|---|
| SystemData | basePath, yearPath, partyPath | Bildordner = basePath/yearPath/partyPath (`~` erlaubt) |
| SystemData | link, password | URL für den QR-Code und zugehöriges Passwort |
| SystemData | eventConfigFile | Name der Nextcloud-Datei, Standard `fotobox.json` |
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

> Die echten Konfigurationsdateien werden per `.gitignore` ausgeschlossen.

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
