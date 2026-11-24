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

    // Waehrend der Diashow liegt ein deckender, bildschirmfuellender Stapel
    // ueber dem Livebild. Jedes Bild, das dann noch umgewandelt wird, ist
    // Arbeit fuer den Papierkorb - und die Box steht einen Abend lang
    // ueberwiegend genau dort.
    //
    // Angehalten wird nur das Umwandeln, nicht die Kamera: ein Neustart der
    // Kamera dauert und kann scheitern. Die Bilder werden weiter entgegen-
    // genommen und verworfen.
    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

signals:
    void frameReady(const QImage& frame);

private:
    QImage m_lastFrame;
    bool m_paused = false;
};
