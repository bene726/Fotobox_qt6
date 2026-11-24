# TODO – Fotobox

## Als Nächstes

### Hardware, offen am Pi 5
- [ ] **LED leuchtet nicht.** Die PWM läuft nachweislich: `pwmchip1` (`pwm_pio@c`, GPIO 12),
      `period` und `duty_cycle` lassen sich von Hand setzen, ohne dass sich etwas tut.
      Das Signal kommt also nicht am Transistor an. Zu prüfen: gemeinsame Masse mit dem
      LED-Netzteil, Signalader wirklich auf Pin 32, Spannung an Pin 32 gegen Pin 30
      messen (0 V bzw. 3,3 V), Steuerspannung des Transistormoduls (manche brauchen 5 V).
      Testbefehl: `echo 1000000 > /sys/class/pwm/pwmchip1/pwm0/duty_cycle`
- [ ] **Taster: Prellen am 7-m-Kabel.** Das Einfrieren ist gefunden und behoben – der
      Ereignis-Deskriptor war blockierend, `read()` wartete im Oberflächen-Thread
      (Commit `514724e`). Übrig bleibt die unruhige Leitung: der interne Pull-up des
      RP1 hat rund 50 kΩ, das reicht bei 7 m nicht. Softwareseitig wird das durch
      400 ms Nachlauf je Druck abgefangen, die Warnung greift erst ab 2000 Flanken
      je Sekunde. Abhilfe an der Hardware steht in [GPIO.md](GPIO.md), Abschnitt
      „Lange Leitung zum Taster“: 2,2 kΩ nach 3,3 V, 100 nF nach GND, 330 Ω in Reihe.
      Zum Nachmessen `fotobox --gpio-watch`, ein Druck genügt.
      **Am Pi noch nicht nachgeprüft.**

### Konfiguration am Pi aufräumen
- [ ] `SystemData`-Block aus `~/.config/fotobox/config.json` entfernen; dort steckt noch
      ein altes `partyPath`, das den Bildordner auf `Mein_Event` festhält
- [ ] `partyPath` und `partyname` gemeinsam in `~/Nextcloud/fotobox.json` setzen

### Sonstiges
- [ ] Autostart auf dem Pi testen (Dienst liegt im Paket, ungetestet)
- [ ] systemd-Watchdog am Pi nachprüfen: nach dem Update
      `systemctl --user show fotobox -p WatchdogUSec -p Restart` (erwartet `WatchdogUSec=30s`)
      und im Protokoll die Zeile „systemd-Watchdog aktiv“ suchen
- [ ] Nextcloud-Anbindung klären: Sync-Client auf dem Pi oder Upload aus der Anwendung
- [ ] Release `v0.1.0` fertigstellen (Umgebungsregel für `v*`, dann Lauf wiederholen)

### Tests
Seit dem Testgerüst (`cmake -DFOTOBOX_TESTS=ON`, dann `ctest`) laufen Tests für
Konfiguration, Bildordner, Designs, QR-Code und Watchdog; die CI fährt sie auf x86_64
und auf arm64 und zeichnet dazu jedes Design offscreen. Was dort noch fehlt:

- [ ] Zustandsmaschine von `MainWindow` testbar machen (Slideshow → Countdown →
      Capturing → ShowPhoto). Sie steckt in 813 Zeilen zusammen mit Layout und
      Theming und ist ohne Bildschirm nicht anfassbar.
- [ ] `TriggerGpio`: Entprellen, Nachlauf und Sturmerkennung hängen direkt an
      `QDateTime::currentMSecsSinceEpoch()` und am Datei-Deskriptor. Mit einer
      injizierten Uhr und einer Liste von Flanken wären genau die Fälle prüfbar,
      die diese Woche zweimal schiefgegangen sind.
- [x] Fehler­übersetzung in `CameraGPhoto` gegen echte gphoto2-Ausgaben geprüft
      (`tests/tst_gphoto.cpp`), ohne Kamera.

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
- [x] Freien Speicherplatz überwachen, Warnung ab 1 GB bzw. 200 MB
- [ ] ~~Akkustand der Kamera anzeigen~~ – entfällt: die Kamera hängt am Netzteil
- [x] Uhrzeit und Datum im Einstellungsfenster
- [ ] Ton beim Countdown (drei Pieptöne, Auslösergeräusch)
- [ ] „Nochmal": letztes Foto verwerfen, statt es zu löschen nach `verworfen/` schieben
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
