#include "TriggerKeyboard.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>

TriggerKeyboard::TriggerKeyboard(QObject* parent)
    : Trigger(parent)
{
    qApp->installEventFilter(this);
}

bool TriggerKeyboard::eventFilter(QObject*, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return false;

    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->isAutoRepeat())
        return false;

    switch (ke->key()) {
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit triggered();
        return true;
    default:
        return false;
    }
}
