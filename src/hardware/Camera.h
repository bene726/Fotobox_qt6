#pragma once
#include <QObject>
#include <QString>

// Fotokamera (DSLR). Die Aufnahme laeuft asynchron:
// capture() kehrt sofort zurueck, das Ergebnis kommt per Signal.
class Camera : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    virtual void capture(const QString& targetDir) = 0;

    // Fragt den Ladestand des Kamera-Akkus ab. Die Antwort kommt per Signal,
    // weil sie bei gphoto2 einen eigenen Prozess braucht. Backends ohne echte
    // Kamera antworten sofort mit "unbekannt".
    //
    // Bewusst kein Dauerabruf: jede Abfrage weckt die Kamera auf, und eine
    // Spiegelreflex, die nie einschlaeft, ist genau das Problem, das die
    // Anzeige verhindern soll.
    virtual void requestBatteryLevel() { emit batteryLevel(-1); }

signals:
    void captured(const QString& path);
    void captureFailed(const QString& error);
    void batteryLevel(int percent);   // -1 = unbekannt

protected:
    // Dateiname fuer eine neue Aufnahme, z.B. "photo_2026-10-26_22-45-12.jpg".
    // Reihenfolge von gross nach klein, damit die Dateien chronologisch
    // sortieren. Keine Doppelpunkte: damit kaemen Windows-Clients an derselben
    // Nextcloud nicht zurecht.
    static QString newFilePath(const QString& dir, const QString& prefix);
};
