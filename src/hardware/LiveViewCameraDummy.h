#pragma once
#include "LiveViewCamera.h"
#include <QTimer>

class LiveViewCameraDummy : public LiveViewCamera
{
    Q_OBJECT
public:
    explicit LiveViewCameraDummy(QObject* parent = nullptr);

    void start() override;
    void stop() override;

private slots:
    void generate();

private:
    QTimer* timer;
};
