#include "Storage.h"
#include "FileWatcher.h"

#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>

namespace {

// Mittel ueber die zuletzt abgelegten Bilder. Nur die letzten paar, damit
// Probeaufnahmen vom Aufbau die Schaetzung nicht dauerhaft verziehen.
constexpr int kSampleCount = 20;

qint64 averagePhotoSize(const QString& dir)
{
    const QStringList files = FileWatcher::imageFiles(dir);   // aeltestes zuerst
    if (files.isEmpty())
        return Storage::kDefaultBytesPerPhoto;

    qint64 sum = 0;
    int n = 0;
    for (int i = files.size() - 1; i >= 0 && n < kSampleCount; --i) {
        const qint64 size = QFileInfo(files.at(i)).size();
        if (size <= 0)
            continue;
        sum += size;
        n++;
    }
    if (n == 0)
        return Storage::kDefaultBytesPerPhoto;
    return qMax<qint64>(1, sum / n);
}

} // namespace

Storage::Level Storage::levelFor(qint64 freeBytes)
{
    if (freeBytes < kCriticalBytes)
        return Level::Critical;
    if (freeBytes < kLowBytes)
        return Level::Low;
    return Level::Ok;
}

Storage::Space Storage::check(const QString& dir)
{
    Space s;
    // Ohne vorhandenen Ordner liefert QStorageInfo den Datentraeger des
    // naechsthoeheren Pfades. Das waere eine Zahl, die nichts mit dem Ziel zu
    // tun hat - lieber gar keine.
    if (dir.isEmpty() || !QDir(dir).exists())
        return s;

    const QStorageInfo info(dir);
    if (!info.isValid() || !info.isReady())
        return s;

    s.known = true;
    // Bewusst nicht QStorageInfo::isReadOnly(): das meldet den Datentraeger,
    // und unter macOS ist das das versiegelte Systemvolume - der Bildordner
    // galt damit immer als schreibgeschuetzt. access(W_OK) fragt genau das,
    // worauf es ankommt, und faengt neben Rechten auch einen nur lesend
    // eingehaengten Stick ab.
    s.readOnly = !QFileInfo(dir).isWritable();
    s.freeBytes = qMax<qint64>(0, info.bytesAvailable());
    s.bytesPerPhoto = averagePhotoSize(dir);
    s.photosLeft = static_cast<int>(
        qMin<qint64>(s.freeBytes / s.bytesPerPhoto, kMaxPhotosLeft));
    s.level = levelFor(s.freeBytes);
    return s;
}

QString Storage::humanBytes(qint64 bytes)
{
    if (bytes < 0)
        return QStringLiteral("?");

    const double kb = bytes / 1024.0;
    if (kb < 1024.0)
        return QString::number(qRound(kb)) + " kB";

    const double mb = kb / 1024.0;
    if (mb < 1024.0)
        return QString::number(qRound(mb)) + " MB";

    const double gb = mb / 1024.0;
    // Komma statt Punkt, der Rest der Oberflaeche ist auch deutsch.
    return QString::number(gb, 'f', 1).replace('.', ',') + " GB";
}
