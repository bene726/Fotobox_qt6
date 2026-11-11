#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QStringList>
#include <QTimer>

struct Config;

// Beobachtet alle Konfigurationsdateien und meldet Aenderungen.
//
// Beobachtet werden auch die Ordner der erwarteten Dateien, damit eine erst
// spaeter angelegte Datei ebenfalls bemerkt wird. Das ist noetig, weil viele
// Programme beim Speichern die Datei ersetzen statt sie zu beschreiben: der
// alte Eintrag zeigt danach ins Leere.
class ConfigWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ConfigWatcher(QObject* parent = nullptr);

    // Pfade neu setzen, nach jedem Laden aufrufen
    void watch(const Config& cfg);

    // Eigene Schreibvorgaenge nicht als Aenderung melden
    void pause(int ms);

signals:
    void changed();

private:
    void onTouched();

    QFileSystemWatcher m_fsw;
    QTimer m_debounce;
    QStringList m_files;
    qint64 m_quietUntil = 0;
};
