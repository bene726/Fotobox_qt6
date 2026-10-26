#include "Camera.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

QString Camera::newFilePath(const QString& dir, const QString& prefix)
{
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString path = QString("%1/%2_%3.jpg").arg(dir, prefix, stamp);

    // Zwei Aufnahmen in derselben Sekunde kann der Ablauf eigentlich nicht
    // erzeugen. Falls doch, lieber anhaengen als das erste Bild verlieren.
    for (int n = 2; QFileInfo::exists(path); ++n)
        path = QString("%1/%2_%3_%4.jpg").arg(dir, prefix, stamp).arg(n);

    return path;
}
