# GPIO & Hardware – Fotobox

Zielsystem: **Raspberry Pi 5** mit Raspberry Pi OS (64-bit).
Der Code läuft auch auf einem Pi 4, die Unterschiede fängt die Anwendung selbst ab.

---

## Überblick

Die Fotobox nutzt zwei GPIO-Funktionen:

| Funktion | Beschreibung | Konfiguration |
|---|---|---|
| Auslöse-Taster | Schließt den Kontakt → Foto | `Hardware.gpioLine` |
| LED-Beleuchtung | PWM-Signal auf einen Transistor | `Hardware.ledPwmChannel` |

---

## Taster

### Verhalten
- Ein Tastendruck löst **immer** ein Foto aus, sofern gerade kein Countdown läuft
- Entspricht dem Signal `triggered()` der Klasse `Trigger`

### Verdrahtung
Standard ist **Taster gegen GND** mit internem Pull-up:

```
GPIO ──────┬────── Taster ────── GND
           │
    interner Pull-up (im SoC)
```

Ruhezustand ist damit High, das Drücken erzeugt eine **fallende Flanke**.
Ein externer Widerstand ist nicht nötig.

Alternativ Taster gegen 3,3 V: dann `"gpioPull": "down"` setzen, ausgewertet wird
die steigende Flanke.

> ⚠️ Niemals gegen 5 V schalten. Die GPIOs vertragen nur 3,3 V.

### Implementierung
- Klasse `TriggerGpio`
- Spricht `/dev/gpiochipN` direkt über die Kernel-Schnittstelle **uAPI v2** an
- Dadurch keine Abhängigkeit zu `libgpiod`, dessen API sich zwischen Version 1 und 2
  grundlegend unterscheidet
- Entprellt bevorzugt **im Kernel**; kann der Treiber das nicht, übernimmt es die Anwendung
- Läuft ereignisgesteuert über den Qt-Eventloop, kein eigener Thread, kein Polling

### Chip-Nummer
Die Nummer von `/dev/gpiochipN` ist **nicht stabil**: auf dem Pi 4 ist der Header
`gpiochip0`, auf dem Pi 5 je nach Kernel `gpiochip0` oder `gpiochip4`.
Deshalb steht `"gpioChip": "auto"` in der Konfiguration. Die Anwendung sucht den Chip
anhand seines Labels (`pinctrl-rp1` auf dem Pi 5, `pinctrl-bcm2711` auf dem Pi 4).

---

## Pin-Nummer herausfinden

Wenn die Belegung einer vorhandenen Verdrahtung unbekannt ist:

```bash
fotobox --gpio-info     # alle Leitungen mit Name und Belegung
fotobox --gpio-watch    # zusätzlich: Flanken anzeigen
```

`--gpio-watch` beobachtet alle freien Header-Leitungen mit internem Pull-up.
Taster drücken, die Ausgabe nennt BCM-Nummer und Pin am Header:

```
20:13:44.218  BCM 17 (Pin 11)  fallend (gedrueckt)
20:13:44.396  BCM 17 (Pin 11)  steigend (losgelassen)
```

Beide Modi brauchen keinen Bildschirm und laufen auch über SSH.

---

## LED-Beleuchtung

Das LED-Band bringt Vorwiderstände mit und wird über einen Transistor geschaltet.
Der Pi liefert nur das **PWM-Signal** an die Basis bzw. das Gate.

### Auf dem Pi 5
`pwm-pio` nutzt die programmierbare IO-Einheit des RP1 und läuft auf **jedem**
GPIO von 0 bis 27, hardware-getaktet:

```
dtoverlay=pwm-pio,gpio=12
```

Die Pinwahl ist damit frei. Bis zu vier solcher Ausgänge sind möglich, solange
nichts anderes die PIO-Einheit belegt.

### Auf dem Pi 4 und älter
Der PWM-Block des SoC bedient nur vier Pins:

| GPIO | Header-Pin | Eintrag |
|---|---|---|
| 12 | 32 | `dtoverlay=pwm,pin=12,func=4` |
| 13 | 33 | `dtoverlay=pwm,pin=13,func=4` |
| 18 | 12 | `dtoverlay=pwm,pin=18,func=2` |
| 19 | 35 | `dtoverlay=pwm,pin=19,func=2` |

### Überall
`dtoverlay=pwm-gpio,gpio=N` erzeugt die PWM im Kernel per Software. Das geht auf
jedem Pin, ist aber am ungenauesten und nur die Notlösung.

In allen drei Fällen ist die Ansteuerung identisch, die Fotobox merkt keinen
Unterschied. `pigpio` funktioniert auf dem Pi 5 übrigens nicht mehr.

Ein konkreter Belegungsvorschlag steht in der [README](README.md#-vorgeschlagene-pinbelegung).

### Aktivieren
In `/boot/firmware/config.txt` eintragen und neu starten:

```
[all]
dtoverlay=pwm-pio,gpio=12
```

Die erlaubten Parameter listet `dtoverlay -h pwm-pio` auf dem Pi selbst auf.

Prüfen:

```bash
ls /sys/class/pwm/        # muss mindestens einen pwmchip zeigen
cat /sys/class/pwm/pwmchip0/npwm
```

### Implementierung
- Klasse `LedPwm`, steuert `/sys/class/pwm/pwmchipN/pwmM`
- `"ledPwmChip": -1` sucht den passenden Chip selbst, da die Nummer je nach
  Kernel und Overlay wechselt
- Frequenz über `"ledPwmFrequency"`, Standard 1000 Hz
- Helligkeit 0–100 % als Einschaltdauer, einstellbar im Menü der Anwendung
- Beim Beenden wird die LED ausgeschaltet

### Rechte
Der Benutzer muss in der Gruppe `gpio` sein, sonst sind weder `/dev/gpiochip*`
noch `/sys/class/pwm` beschreibbar:

```bash
sudo usermod -aG gpio "$USER"   # danach neu anmelden
```

---

## Touchscreen
- Touch ist **kein GPIO**
- Wird über Qt-Events erkannt
- Touch beendet nur die Slideshow und löst **nie** ein Foto aus

---

## Sicherheit
- GPIOs niemals direkt im UI-Thread pollen; hier löst der Kernel das über Ereignisse
- Entprellen beachten, Standard sind 30 ms
- Die Last des LED-Bands läuft über den Transistor, nicht über den GPIO
