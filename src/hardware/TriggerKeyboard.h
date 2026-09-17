#pragma once
#include "Trigger.h"

// Leertaste oder Enter loest ein Foto aus.
// Fuer die Entwicklung am Host und als Fallback am Pi (USB-Taster/Fusspedal).
class TriggerKeyboard : public Trigger
{
    Q_OBJECT
public:
    explicit TriggerKeyboard(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};
