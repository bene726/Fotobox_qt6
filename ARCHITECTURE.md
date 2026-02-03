# Fotobox – Architektur

## Überblick

Die Fotobox ist bewusst modular aufgebaut. Ziel ist es, Hardware, UI und Logik
klar zu trennen, um Wartbarkeit und Erweiterbarkeit sicherzustellen.

---

## Zentrale Konzepte

### 1. Livebild (Basis-Ebene)
- Raspberry Pi Kamera (CSI)
- Läuft permanent
- Wird niemals gestoppt
- Liefert kontinuierlich Frames an das UI

### 2. Overlays (UI-Ebene)
- Slideshow
- Countdown
- Fotoanzeige

Diese liegen immer **über** dem Livebild.

---

## Kamera-Trennung

### LiveViewCamera
- Zuständig für das Livebild
- Keine Dateierzeugung
- Liefert QImage/QPixmap

### Camera
- Zuständig für Fotoaufnahmen
- DSLR via gphoto2
- Genau ein Bild pro Auslösung

---

## Eingaben

### TouchInput
- Reagiert auf Touch-Ereignisse
- Beendet Slideshow
- Startet niemals ein Foto

### TriggerThread
- Physischer Button (GPIO)
- Startet Countdown + Foto

---

## Steuerung

### MainWindow
- Zentrale Steuerinstanz
- Kennt alle Zustände
- Verbindet Signale
- Enthält keine Hardware-Details

---

## Zustandsmodell (vereinfacht)

- Slideshow
- Countdown
- Fotoanzeige
- LiveView

Der Wechsel erfolgt ausschließlich ereignisgesteuert.

---

## Design-Ziele

- Keine Blockierung des UI-Threads
- Keine Kamera-Neustarts
- Klare Zuständigkeiten
- Erweiterbar (LED, Sound, Upload-Status)

---

## Nicht-Ziele

- Dauerbetrieb
- Hochverfügbarkeit
- Cloud-Enterprise-Funktionen

---

## Fazit

Die Architektur ist bewusst einfach gehalten, aber stabil genug für reale Events.
