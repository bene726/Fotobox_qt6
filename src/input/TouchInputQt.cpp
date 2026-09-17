#include "TouchInputQt.h"

#include <QCoreApplication>
#include <QEvent>
#include <QMouseEvent>

TouchInputQt::TouchInputQt(QObject* parent)
    : TouchInput(parent)
{
    m_sinceLast.start();
    qApp->installEventFilter(this);
}

bool TouchInputQt::eventFilter(QObject*, QEvent* event)
{
    bool isTouch = false;

    if (event->type() == QEvent::TouchBegin) {
        isTouch = true;
    } else if (event->type() == QEvent::MouseButtonPress) {
        // Von Qt aus Touch synthetisierte Mausklicks ignorieren,
        // sonst kommt jeder Touch doppelt an.
        auto* me = static_cast<QMouseEvent*>(event);
        isTouch = (me->source() == Qt::MouseEventNotSynthesized);
    }

    if (!isTouch)
        return false;

    // Entprellen
    if (m_sinceLast.elapsed() < kDebounceMs)
        return false;
    m_sinceLast.restart();

    emit touched();
    return false;
}
