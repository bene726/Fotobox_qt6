#pragma once
#include "TouchInput.h"
#include <QElapsedTimer>

// Erkennt Touch (und am Host: Mausklick) ueber einen globalen Event-Filter.
// Installiert sich selbst auf der QApplication.
class TouchInputQt : public TouchInput
{
    Q_OBJECT
public:
    explicit TouchInputQt(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    static constexpr int kDebounceMs = 300;
    QElapsedTimer m_sinceLast;
};
