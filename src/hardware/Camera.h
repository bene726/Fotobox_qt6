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

signals:
    void captured(const QString& path);
    void captureFailed(const QString& error);
};
