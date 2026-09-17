#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

// Beobachtet den Bildordner und meldet neu erschienene Bilddateien.
// Bilddateien werden case-insensitiv anhand der Endung erkannt.
class FileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit FileWatcher(const QString& dir, QObject* parent = nullptr);

    // Alle Bilddateien im Ordner als absolute Pfade, aelteste zuerst.
    static QStringList imageFiles(const QString& dir);
    static bool isImageFile(const QString& path);

signals:
    void newFile(const QString& path);

private:
    void scan(bool emitNew);

    QString m_dir;
    QSet<QString> m_known;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
