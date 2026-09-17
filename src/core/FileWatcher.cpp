#include "FileWatcher.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

FileWatcher::FileWatcher(const QString& dir, QObject* parent)
    : QObject(parent)
    , m_dir(dir)
{
    if (!QDir().mkpath(m_dir))
        qWarning() << "Bildordner konnte nicht angelegt werden:" << m_dir;

    // Erster Scan ohne Signale: vorhandene Dateien sind bekannt.
    scan(false);

    // Kurze Verzoegerung, damit halb geschriebene Dateien nicht sofort gemeldet werden.
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(500);
    connect(&m_debounce, &QTimer::timeout, this, [this] { scan(true); });

    if (!m_watcher.addPath(m_dir))
        qWarning() << "Ordner kann nicht beobachtet werden:" << m_dir;
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString&) { m_debounce.start(); });
}

bool FileWatcher::isImageFile(const QString& path)
{
    static const QStringList suffixes = {"jpg", "jpeg", "png"};
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

QStringList FileWatcher::imageFiles(const QString& dir)
{
    QStringList result;
    const QFileInfoList entries = QDir(dir).entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    for (const QFileInfo& fi : entries)
        if (isImageFile(fi.fileName()))
            result << fi.absoluteFilePath();
    return result;
}

void FileWatcher::scan(bool emitNew)
{
    const QStringList files = imageFiles(m_dir);
    for (const QString& f : files) {
        if (m_known.contains(f))
            continue;
        m_known.insert(f);
        if (emitNew) {
            qInfo() << "Neue Datei:" << f;
            emit newFile(f);
        }
    }
}
