# TODO – Fotobox

## Als Nächstes

### Hardware, offen am Pi 5
- [ ] **LED leuchtet nicht.** Die PWM läuft nachweislich: `pwmchip1` (`pwm_pio@c`, GPIO 12),
      `period` und `duty_cycle` lassen sich von Hand setzen, ohne dass sich etwas tut.
      Das Signal kommt also nicht am Transistor an. Zu prüfen: gemeinsame Masse mit dem
      LED-Netzteil, Signalader wirklich auf Pin 32, Spannung an Pin 32 gegen Pin 30
      messen (0 V bzw. 3,3 V), Steuerspannung des Transistormoduls (manche brauchen 5 V).
      Testbefehl: `echo 1000000 > /sys/class/pwm/pwmchip1/pwm0/duty_cycle`
- [ ] **Taster erzeugt einen Flankensturm.** Beim Verbinden fror die Oberfläche ein, SSH
      blieb erreichbar. Die Leseschleife ohne Grenze ist behoben, die Leitung ist damit
      aber nicht ruhig. Nach dem Update auf die Warnung „Mehr als N Flanken je Sekunde“
      im Protokoll achten; falls sie kommt, `pinctrl get 17` prüfen und notfalls einen
      Widerstand von 10 kΩ zwischen Pin 11 und 3,3 V setzen.

### Konfiguration am Pi aufräumen
- [ ] `SystemData`-Block aus `~/.config/fotobox/config.json` entfernen; dort steckt noch
      ein altes `partyPath`, das den Bildordner auf `Mein_Event` festhält
- [ ] `partyPath` und `partyname` gemeinsam in `~/Nextcloud/fotobox.json` setzen

### Sonstiges
- [ ] Autostart auf dem Pi testen (Dienst liegt im Paket, ungetestet)
- [ ] Nextcloud-Anbindung klären: Sync-Client auf dem Pi oder Upload aus der Anwendung
- [ ] Release `v0.1.0` fertigstellen (Umgebungsregel für `v*`, dann Lauf wiederholen)

---

## Kurzfristig
- [x] Touch-Debounce verbessern
- [x] Countdown konfigurierbar machen
- [x] Case-insensitive Dateifilter (JPG / PNG)
- [x] QR-Code optional ein-/ausblendbar
- [x] gphoto2-Aufnahme mit der Canon EOS 600D getestet (2,7 s bei Large Fine)
- [x] Schriften in `fonts/` abgelegt, alle Designs nutzen ihre vorgesehenen Familien
- [x] LED-PWM implementiert (sysfs)
- [~] Taster und LED am Pi 5 angeschlossen, beide noch nicht funktionsfähig, siehe oben

---

## Mittelfristig
- [x] Diashow abschaltbar, Wartezeit im Menü einstellbar
- [x] Fotoshow als Stapel von Abzügen, damit sie vom Livebild unterscheidbar ist
- [x] Livebild der USB-Webcam auf dem Pi getestet
- [ ] rpicam-vid LiveView ist vorhanden, aber ungetestet (nur nötig mit Pi-Kamera)
- [x] GPIO-Taster implementiert (Kernel-uAPI v2, kein libgpiod nötig)
- [ ] Bildschirmtastatur (Qt Virtual Keyboard) auf dem Pi
- [ ] Upload-Status-Overlay (Nextcloud)
- [x] Farbe des Ausschalt-Knopfes an das jeweilige Design angepasst
- [x] Auslöser auf dem Touchscreen
- [x] Kamera im Einstellungsmenü wählbar
- [ ] Galerie-Ansicht auf dem Touchscreen

---

## Langfristig / Optional
- [ ] Mehrsprachigkeit
- [ ] Themes / Skins
- [ ] Remote-Wartung
- [ ] Statistiken (Anzahl Fotos)

---

## Nicht geplant
- Dauerbetrieb
- Benutzerverwaltung
- Cloud-Enterprise-Funktionen
