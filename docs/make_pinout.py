#!/usr/bin/env python3
"""Erzeugt docs/pinout.svg: der 40-polige Header mit den Anschluessen der Fotobox.

Aufruf aus dem Projektverzeichnis:  python3 docs/make_pinout.py

Aendert sich die Verdrahtung, hier BENUTZT anpassen und neu erzeugen, statt
das SVG von Hand zu bearbeiten.
""" 
import html

PINS = {
    1:("3V3","pwr3"),    2:("5V","pwr5"),
    3:("GPIO2","bus"),   4:("5V","pwr5"),
    5:("GPIO3","bus"),   6:("GND","gnd"),
    7:("GPIO4","gpio"),  8:("GPIO14","bus"),
    9:("GND","gnd"),    10:("GPIO15","bus"),
   11:("GPIO17","gpio"),12:("GPIO18","pwm"),
   13:("GPIO27","gpio"),14:("GND","gnd"),
   15:("GPIO22","gpio"),16:("GPIO23","gpio"),
   17:("3V3","pwr3"),   18:("GPIO24","gpio"),
   19:("GPIO10","bus"), 20:("GND","gnd"),
   21:("GPIO9","bus"),  22:("GPIO25","gpio"),
   23:("GPIO11","bus"), 24:("GPIO8","bus"),
   25:("GND","gnd"),    26:("GPIO7","bus"),
   27:("ID_SD","id"),   28:("ID_SC","id"),
   29:("GPIO5","gpio"), 30:("GND","gnd"),
   31:("GPIO6","gpio"), 32:("GPIO12","pwm"),
   33:("GPIO13","pwm"), 34:("GND","gnd"),
   35:("GPIO19","pwm"), 36:("GPIO16","gpio"),
   37:("GPIO26","gpio"),38:("GPIO20","gpio"),
   39:("GND","gnd"),    40:("GPIO21","gpio"),
}
FARBE = {"pwr3":"#e8a33d","pwr5":"#cf3a3a","gnd":"#3f4651","gpio":"#2f7d4f",
         "pwm":"#1f6f8b","bus":"#8a8f98","id":"#b8bcc2"}
TEXT   = {"id":"#333"}
NAME   = {"pwr3":"3,3 V","pwr5":"5 V","gnd":"Masse (GND)","gpio":"freier GPIO",
          "pwm":"GPIO mit Hardware-PWM","bus":"belegt (I²C, SPI, serielle Konsole)",
          "id":"reserviert (EEPROM)"}

BENUTZT = {9:"Masse für den Taster", 11:"Signal Taster",
           30:"Masse für die LED",   32:"PWM-Signal LED"}

X0, DX, R = 150, 72, 20
Y_ODD, Y_EVEN = 300, 380
W, H = 1620, 940
e = html.escape

s = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
     f'font-family="DejaVu Sans, Verdana, sans-serif">',
     f'<rect width="{W}" height="{H}" fill="#ffffff"/>']

s.append(f'<text x="{W//2}" y="52" text-anchor="middle" font-size="30" font-weight="bold" '
         f'fill="#1a1a1a">Fotobox: Anschlüsse am 40-poligen Header</text>')
s.append(f'<text x="{W//2}" y="84" text-anchor="middle" font-size="17" fill="#555">'
         f'Blick von oben auf die Platine. Jeder Pin trägt seine Nummer, zählen schlägt schätzen.</text>')

# Platine als Hintergrund
s.append(f'<rect x="{X0-46}" y="{Y_ODD-R-28}" width="{DX*19+92}" height="{Y_EVEN-Y_ODD+2*R+56}" '
         f'rx="14" fill="#f3f4f6" stroke="#d6d8db" stroke-width="2"/>')

def pin(n, idx, oben):
    x = X0 + idx*DX
    y = Y_ODD if oben else Y_EVEN
    label, art = PINS[n]
    fill = FARBE[art]
    out = []
    if n in BENUTZT:   # Markierung zuerst, damit sie hinter dem Pin liegt
        out.append(f'<circle cx="{x}" cy="{y}" r="{R+9}" fill="none" stroke="#d6223c" stroke-width="4"/>')
    if n == 1:         # Pin 1 ist auf der Platine quadratisch
        out.append(f'<rect x="{x-R}" y="{y-R}" width="{2*R}" height="{2*R}" fill="{fill}" '
                   f'stroke="#1a1a1a" stroke-width="3"/>')
    else:
        out.append(f'<circle cx="{x}" cy="{y}" r="{R}" fill="{fill}" stroke="#ffffff" stroke-width="2"/>')
    out.append(f'<text x="{x}" y="{y+6}" text-anchor="middle" font-size="16" font-weight="bold" '
               f'fill="{TEXT.get(art,"#ffffff")}">{n}</text>')
    ly = y - R - 14 if oben else y + R + 22
    out.append(f'<text x="{x}" y="{ly}" text-anchor="middle" font-size="14" fill="#333">{e(label)}</text>')
    return "".join(out)

for i in range(20):
    s.append(pin(2*i+1, i, True))
    s.append(pin(2*i+2, i, False))

s.append(f'<text x="{X0-52}" y="{Y_ODD+6}" text-anchor="end" font-size="15" fill="#666">ungerade</text>')
s.append(f'<text x="{X0-52}" y="{Y_EVEN+6}" text-anchor="end" font-size="15" fill="#666">gerade</text>')
s.append(f'<text x="{X0}" y="{Y_ODD-R-42}" text-anchor="middle" font-size="14" fill="#d6223c" '
         f'font-weight="bold">Pin 1, eckig</text>')

def kasten(x, y, breite, titel, zeilen, ziel_x1, ziel_x2, ziel_y, von_unten):
    h = 44 + 24*len(zeilen)
    o = [f'<rect x="{x}" y="{y}" width="{breite}" height="{h}" rx="10" fill="#fff5f6" '
         f'stroke="#d6223c" stroke-width="2.5"/>',
         f'<text x="{x+16}" y="{y+28}" font-size="18" font-weight="bold" fill="#d6223c">{e(titel)}</text>']
    for i, z in enumerate(zeilen):
        o.append(f'<text x="{x+16}" y="{y+54+i*24}" font-size="15" fill="#333">{e(z)}</text>')
    ay = y + h if von_unten else y
    for zx in (ziel_x1, ziel_x2):
        o.append(f'<path d="M {x+breite/2} {ay} L {zx} {ziel_y}" stroke="#d6223c" '
                 f'stroke-width="2" stroke-dasharray="6 4" fill="none"/>')
    return "".join(o)

s.append(kasten(250, 112, 340, "Auslöse-Taster",
                ["Pin 11 → GPIO 17 (Signal)", "Pin 9  → Masse", "kein Widerstand nötig"],
                X0+4*DX, X0+5*DX, Y_ODD-R-30, True))
s.append(kasten(880, 486, 420, "LED-Beleuchtung (Transistor)",
                ["Pin 32 → GPIO 12 (PWM-Signal)", "Pin 30 → Masse",
                 "dtoverlay=pwm-pio,gpio=12"],
                X0+14*DX, X0+15*DX, Y_EVEN+R+30, False))

# Legende
ly = 660
s.append(f'<text x="90" y="{ly}" font-size="18" font-weight="bold" fill="#1a1a1a">Legende</text>')
for i, art in enumerate(["pwr3","pwr5","gnd","gpio","pwm","bus","id"]):
    y = ly + 30 + i*28
    s.append(f'<circle cx="104" cy="{y-5}" r="11" fill="{FARBE[art]}"/>')
    s.append(f'<text x="126" y="{y}" font-size="15" fill="#333">{e(NAME[art])}</text>')
s.append(f'<circle cx="104" cy="{ly+30+7*28-5}" r="11" fill="none" stroke="#d6223c" stroke-width="3"/>')
s.append(f'<text x="126" y="{ly+30+7*28}" font-size="15" fill="#333">von der Fotobox benutzt</text>')

hinweise = [
    "Pin 1 erkennst du auf der Platine am eckigen Lötauge, in der Nähe der Beschriftung J8.",
    "Beide Verbindungen liegen jeweils in derselben Reihe nebeneinander: ein zweipoliger Stecker genügt.",
    "Der Taster schaltet gegen Masse, der interne Pull-up ist aktiv. Niemals gegen 5 V schalten.",
    "Die LED hängt am Transistor, der GPIO liefert nur das Signal.",
]
for i, z in enumerate(hinweise):
    s.append(f'<text x="600" y="{ly+34+i*30}" font-size="15" fill="#444">• {e(z)}</text>')

s.append('</svg>')
open("docs/pinout.svg","w").write("\n".join(s))
print("geschrieben")
