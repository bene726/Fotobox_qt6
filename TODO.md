# TODO – Fotobox

## Als Nächstes
- [ ] Taster und LED verdrahten, Pin mit `--gpio-watch` bestimmen, `dtoverlay=pwm-pio` eintragen
- [ ] Autostart einrichten (in der README beschrieben, noch nicht getestet)
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
- [ ] Taster und LED auf dem Pi 5 testen

---

## Mittelfristig
- [x] Diashow abschaltbar, Wartezeit im Menü einstellbar
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
