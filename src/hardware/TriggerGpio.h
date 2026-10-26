#pragma once
#include "Trigger.h"

#include <QString>

class QSocketNotifier;

// Physischer Taster am GPIO (Raspberry Pi).
//
// Spricht die Zeichengeraete-Schnittstelle des Kernels direkt an
// (/dev/gpiochipN, uAPI v2). Das vermeidet eine Abhaengigkeit zu libgpiod,
// dessen API sich zwischen Version 1 (Bookworm) und 2 (Trixie) unterscheidet.
//
// Verdrahtung: Taster zwischen GPIO und GND, interner Pull-up aktiv
// (pull = "up", Standard). Alternativ Taster gegen 3,3 V mit pull = "down".
class TriggerGpio : public Trigger
{
    Q_OBJECT
public:
    // chip: "auto" sucht den Header-Chip selbst, sonst z.B. "gpiochip0"
    // pull: "up" | "down" | "none"
    TriggerGpio(const QString& chip, int line, const QString& pull, int debounceMs,
                QObject* parent = nullptr);
    ~TriggerGpio() override;

    bool isReady() const { return m_fd >= 0; }

    // Pfad des Chips, der die Header-Pins bedient ("" wenn keiner gefunden wurde).
    static QString findHeaderChip();

private:
    void onEvent();

    int m_fd = -1;                       // Datei-Deskriptor der angeforderten Leitung
    QSocketNotifier* m_notifier = nullptr;
    bool m_softwareDebounce = false;     // falls der Kernel kein Entprellen kann
    int m_debounceMs = 30;
    qint64 m_lastEventMs = 0;
};
