#pragma once
#include "TouchInput.h"
#include <QObject>

class TouchInputQt : public TouchInput
{
    Q_OBJECT
public:
    explicit TouchInputQt(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};
