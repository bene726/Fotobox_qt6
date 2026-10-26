#pragma once
#include <QImage>
#include <QObject>

// Livebild-Quelle. Laeuft dauerhaft und liefert Frames per Signal.
// Merkt sich den letzten Frame, damit andere Module (z.B. Snapshot-Kamera)
// darauf zugreifen koennen.
class LiveViewCamera : public QObject
{
    Q_OBJECT
public:
    explicit LiveViewCamera(QObject* parent = nullptr);
    ~LiveViewCamera() override;

    virtual void start() = 0;
    virtual void stop() = 0;

    // Andere Kamera waehlen. Liefert false, wenn das Backend das nicht kann.
    virtual bool setDevice(const QString& device) { Q_UNUSED(device); return false; }
    virtual bool canSetDevice() const { return false; }

    QImage lastFrame() const { return m_lastFrame; }

signals:
    void frameReady(const QImage& frame);

private:
    QImage m_lastFrame;
};
