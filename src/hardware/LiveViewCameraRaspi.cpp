class LiveViewCameraRaspi : public LiveViewCamera
{
    Q_OBJECT
public:
    void start() override {
        // Kamera initialisieren
        // Stream starten
    }

    void stop() override {
        // optional, meist nie aufgerufen
    }

signals:
    void frameReady(const QImage& frame);
};
