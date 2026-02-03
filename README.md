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
├── core/                 # Konfiguration, Dateiwatcher
│   ├── Config.*
│   └── FileWatcher.*
│
├── hardware/             # Kameras & Hardware
│   ├── Camera.*          # DSLR (gphoto2 / Dummy)
│   ├── LiveViewCamera.*  # Raspi Cam (Livebild)
│   └── TriggerThread.*   # Button
│
├── input/                # Eingaben
│   └── TouchInput.*
│
├── ui/                   # UI-Komponenten
│   ├── QrCodeWidget.*
│   └── qrcodegen.*
│
├── MainWindow.*
└── main.cpp
```

---

## 🛠️ Build (Host)

Getestet unter **Ubuntu 24.xx**.

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

---

## ⚙️ Konfiguration

Die Anwendung nutzt eine **lokale JSON-Konfigurationsdatei**,  
die bewusst **nicht** im Repository liegt.

### Beispiel: `Config.example.json`

```json
{
  "SystemData": {
    "basePath": "/home/fotobox/Nextcloud/",
    "yearPath": "2026/",
    "partyPath": "Mein_Event/",
    "partyname": "Mein_Event",
    "link": "https://example.com",
    "password": "secret"
  }
}
```

> Die echte `Config.js` / `config.json` wird per `.gitignore` ausgeschlossen.

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
