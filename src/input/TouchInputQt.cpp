#include "TouchInputQt.h"
#include <QEvent>
#include <QMouseEvent>
#include <QTouchEvent>

TouchInputQt::TouchInputQt(QObject* parent)
    : TouchInput(parent)
{
}

bool TouchInputQt::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::MouseButtonPress) {
        emit touched();
    }
    return false;
}
