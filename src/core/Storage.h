#pragma once
#include <QString>

// Platz auf dem Datentraeger des Bildordners.
//
// Eine volle Karte ist der haeufigste stille Ausfall einer Fotobox: die
// Aufnahme scheitert erst in dem Moment, in dem jemand davorsteht. Deshalb
// wird der Platz im Betrieb beobachtet und nicht erst beim Schreiben.
namespace Storage {

enum class Level {
    Ok,         // reicht
    Low,        // wird knapp, Hinweis anzeigen
    Critical,   // gleich vorbei, deutlich warnen
};

// Ab hier wird gewarnt. Bewusst grosszuegig: wer erst bei 100 MB merkt, dass
// die Karte voll ist, kann am Abend nichts mehr dagegen tun.
constexpr qint64 kLowBytes = 1024LL * 1024 * 1024;        // 1 GB
constexpr qint64 kCriticalBytes = 200LL * 1024 * 1024;    // 200 MB

// Schaetzung, solange noch keine Fotos im Ordner liegen. Eine Canon EOS 600D
// legt bei "Large Fine" rund 7 MB je Bild ab.
constexpr qint64 kDefaultBytesPerPhoto = 7LL * 1024 * 1024;

// Obergrenze der geschaetzten Anzahl. Wer so viel Platz hat, braucht keine
// Zahl mehr, und eine sechsstellige waere in der Statuszeile nur Unsinn.
constexpr int kMaxPhotosLeft = 99999;

struct Space
{
    bool known = false;        // false: Ordner fehlt oder ist nicht lesbar
    bool readOnly = false;     // z.B. ein nur lesend eingehaengter USB-Stick
    qint64 freeBytes = 0;
    qint64 bytesPerPhoto = kDefaultBytesPerPhoto;
    int photosLeft = 0;
    Level level = Level::Ok;

    // Stoesst die Schaetzung an die Obergrenze, ist sie keine Auskunft mehr.
    // Beide Anzeigen - Statuszeile und Meldung - lassen sie dann weg.
    bool photosLeftMeaningful() const { return known && photosLeft < kMaxPhotosLeft; }
};

// Sieht nach, wie viel Platz der Ordner noch hat, und schaetzt daraus die
// Anzahl der Fotos. Die Schaetzung nimmt die Groesse der zuletzt abgelegten
// Bilder, damit sie zur wirklich benutzten Kamera passt.
Space check(const QString& dir);

Level levelFor(qint64 freeBytes);

// "3,4 GB", "820 MB" - deutsche Schreibweise, wie der Rest der Oberflaeche.
QString humanBytes(qint64 bytes);

} // namespace Storage
