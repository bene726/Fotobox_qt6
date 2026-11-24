#include "FrameGate.h"

void FrameGate::setFps(int fps)
{
    m_fps = fps;
    // Abgerundet, damit eine Kamera, die genau die gewuenschte Rate liefert,
    // nicht an der Rundung haengenbleibt: 30 fps kommen alle 33,3 ms, die
    // Schranke steht dann bei 33 ms.
    m_minGapMs = fps > 0 ? 1000 / fps : 0;
    reset();
}

void FrameGate::reset()
{
    m_nextMs = -1;
}

bool FrameGate::accept(qint64 nowMs)
{
    if (m_minGapMs <= 0)
        return true;

    if (m_nextMs < 0) {
        m_nextMs = nowMs + m_minGapMs;
        return true;
    }
    if (nowMs < m_nextMs)
        return false;

    // Haengt die Verarbeitung weit zurueck - weil gerade ein Foto gemacht
    // wurde, etwa -, nicht versuchen aufzuholen. Sonst kaeme danach ein
    // Schwall Bilder auf einmal.
    m_nextMs = (nowMs - m_nextMs > m_minGapMs) ? nowMs + m_minGapMs
                                               : m_nextMs + m_minGapMs;
    return true;
}
