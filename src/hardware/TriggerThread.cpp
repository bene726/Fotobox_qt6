#include "TriggerThread.h"
#include <QThread>

void TriggerThread::run()
{
    while (true) {
        QThread::sleep(5); // später GPIO
        emit triggered();
    }
}