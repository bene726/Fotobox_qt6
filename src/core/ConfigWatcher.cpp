#include "ConfigWatcher.h"
#include "Config.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

ConfigWatcher::ConfigWatcher(QObject* parent)
    : QObject(parent)
{
    // Editoren schreiben in mehreren Schritten, deshalb kurz sammeln.
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(600);
    connect(&m_debounce, &QTimer::timeout, this, [this] {
        if (QDateTime::currentMSecsSinceEpoch() < m_quietUntil)
            return;
        emit changed();
    });

    connect(&m_fsw, &QFileSystemWatcher::fileChanged, this, &ConfigWatcher::onTouched);
    connect(&m_fsw, &QFileSystemWatcher::directoryChanged, this, &ConfigWatcher::onTouched);
}

void ConfigWatcher::pause(int ms)
{
    m_quietUntil = QDateTime::currentMSecsSinceEpoch() + ms;
}

void ConfigWatcher::onTouched()
{
    m_debounce.start();
}

void ConfigWatcher::watch(const Config& cfg)
{
    // Alle Ebenen, auch die noch nicht vorhandenen.
    QStringList dateien = cfg.loadedFrom;
    for (const QString& p : {Config::systemPath(), Config::defaultSavePath(), cfg.eventConfigPath()})
        if (!p.isEmpty() && !dateien.contains(p))
            dateien << p;

    if (!m_fsw.files().isEmpty())
        m_fsw.removePaths(m_fsw.files());
    if (!m_fsw.directories().isEmpty())
        m_fsw.removePaths(m_fsw.directories());

    QStringList ordner;
    for (const QString& p : dateien) {
        if (QFileInfo::exists(p))
            m_fsw.addPath(p);
        const QString d = QFileInfo(p).absolutePath();
        if (!ordner.contains(d) && QFileInfo::exists(d))
            ordner << d;
    }
    if (!ordner.isEmpty())
        m_fsw.addPaths(ordner);

    m_files = dateien;
    qInfo().noquote() << "Beobachte Konfigurationsdateien:" << dateien.join(", ");
}
