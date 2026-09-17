#pragma once
#include <QObject>

// Steuert die LED-Beleuchtung (Helligkeit 0..100 %).
class LedController : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    int brightness() const { return m_brightness; }

public slots:
    void setBrightness(int percent);

signals:
    void brightnessChanged(int percent);

protected:
    // Von der Implementierung ueberschrieben, um die Hardware zu setzen.
    virtual void apply(int percent) = 0;

private:
    int m_brightness = -1;
};

// Ohne Hardware, gibt nur Log-Ausgaben.
class LedDummy : public LedController
{
    Q_OBJECT
public:
    using LedController::LedController;

protected:
    void apply(int percent) override;
};
