# GPIO & Hardware – Fotobox

## Überblick
Die Fotobox nutzt GPIOs für:
- Auslöse-Button
- optionale LEDs / Blitz

---

## Button

### Verhalten
- Ein Tastendruck löst **immer** ein Foto aus
- Entspricht dem Signal `triggered()`

### Implementierung
- Interface: `Trigger` (Signal `triggered()`)
- `TriggerGpio`: Button am Pi über libgpiod, **noch Platzhalter**
- `TriggerKeyboard`: Leertaste/Enter, für Host und als Fallback am Pi
- `TriggerTimer`: automatisches Auslösen für Tests
- Auswahl über `Hardware.trigger` in der Konfiguration
- Kein UI-Code im GPIO-Modul

### Pi 5
- Kein sysfs-GPIO mehr, libgpiod verwenden
- Chip und Line sind konfigurierbar (`gpioChip`, `gpioLine`)

---

## Touchscreen
- Touch ist **kein GPIO**
- Wird über Qt Events erkannt
- Touch beendet nur die Slideshow

---

## LEDs / Blitz (optional)

### Empfohlenes Verhalten
- Weißes Aufblenden bei Countdown = 0
- PWM-gesteuerte LED-Helligkeit
- Konfigurierbare Standardhelligkeit

### Erweiterungspunkte
- eigener `LedController`
- Ansteuerung über GPIO oder I2C

---

## Sicherheit
- GPIOs niemals direkt im UI-Thread lesen
- Entprellen (Debounce) beachten
