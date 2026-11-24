#pragma once
#include <QtGlobal>

// Laesst hoechstens so viele Bilder je Sekunde durch, wie eingestellt sind.
//
// Gedacht fuer Kameras, deren Bildrate sich nicht vorgeben laesst. Eine
// USB-Webcam am Qt-Backend liefert, was ihr Format hergibt - meist 30 Bilder
// je Sekunde. Jedes davon zu dekodieren kostet den ganzen Abend Rechenzeit,
// und fuer eine Vorschau, in die jemand hineinschaut, sind 30 Bilder mehr als
// genug.
//
// Entscheidend ist, dass gefragt wird, bevor das Bild umgewandelt wird: das
// Umwandeln ist der teure Teil, nicht das Entgegennehmen.
//
// Die Uhr wird hereingereicht, damit sich das Verhalten ohne Warten pruefen
// laesst.
class FrameGate
{
public:
    // 0 oder weniger schaltet die Begrenzung ab: dann kommt jedes Bild durch.
    void setFps(int fps);
    int fps() const { return m_fps; }

    // true, wenn dieses Bild verarbeitet werden soll.
    bool accept(qint64 nowMs);

    void reset();

private:
    int m_fps = 0;
    qint64 m_minGapMs = 0;
    qint64 m_nextMs = -1;
};
