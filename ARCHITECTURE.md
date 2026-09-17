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
- DSLR via gphoto2 (als eigener Prozess, blockiert das UI nicht)
- Genau ein Bild pro Auslösung
- Ergebnis kommt asynchron per Signal `captured()` / `captureFailed()`

---

## Eingaben

### TouchInput
- Reagiert auf Touch-Ereignisse
- Beendet Slideshow
- Startet niemals ein Foto

### Trigger
- Abstrakter Auslöser mit Signal `triggered()`
- Implementierungen: GPIO-Button (Pi), Tastatur, Timer (Tests)
- Startet Countdown + Foto

---

## Steuerung

### MainWindow
- Zentrale Steuerinstanz
- Kennt alle Zustände
- Verbindet Signale
- Enthält keine Hardware-Details

---

## Zustandsmodell

```
Slideshow --Touch--> LiveView --Timeout--> Slideshow
Slideshow/LiveView --Button--> Countdown --0--> Capturing
Capturing --captured--> ShowPhoto --Timeout--> LiveView
Capturing --captureFailed--> LiveView (mit Fehlermeldung)
```

Der Wechsel erfolgt ausschließlich ereignisgesteuert. Während Countdown,
Capturing und ShowPhoto werden weitere Auslöser ignoriert.

### Hardware-Auswahl
`HardwareFactory` wählt die Backends anhand der Konfiguration.
`auto` bedeutet: echte Hardware im Pi-Build, Dummies und Tastatur am Host.

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
