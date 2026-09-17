#pragma once
#include <QObject>

// Ausloeser fuer ein Foto (physischer Button, Tastatur, ...).
class Trigger : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

signals:
    void triggered();
};
