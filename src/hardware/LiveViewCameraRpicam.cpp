#include "LiveViewCameraRpicam.h"

#include <QDebug>
#include <QImage>

static const QByteArray kSoi = QByteArray::fromHex("FFD8");
static const QByteArray kEoi = QByteArray::fromHex("FFD9");

LiveViewCameraRpicam::LiveViewCameraRpicam(const QString& binary, int width, int height,
                                           int fps, QObject* parent)
    : LiveViewCamera(parent)
    , m_binary(binary)
    , m_width(width)
    , m_height(height)
    , m_fps(fps)
{
    m_proc.setReadChannel(QProcess::StandardOutput);
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, &LiveViewCameraRpicam::onReadyRead);
    connect(&m_proc, &QProcess::finished, this, &LiveViewCameraRpicam::onFinished);
    connect(&m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            qWarning() << "rpicam-vid nicht gefunden:" << m_binary;
    });

    m_restart.setSingleShot(true);
    m_restart.setInterval(2000);
    connect(&m_restart, &QTimer::timeout, this, [this] {
        if (m_running)
            start();
    });
}

LiveViewCameraRpicam::~LiveViewCameraRpicam()
{
    stop();
}

void LiveViewCameraRpicam::start()
{
    m_running = true;
    if (m_proc.state() != QProcess::NotRunning)
        return;

    m_buffer.clear();
    const QStringList args = {
        "--codec", "mjpeg",
        "--timeout", "0",
        "--nopreview",
        "--width", QString::number(m_width),
        "--height", QString::number(m_height),
        "--framerate", QString::number(m_fps),
        "--flush",
        "--output", "-",
    };
    qInfo() << "Starte Livebild:" << m_binary << args.join(' ');
    m_proc.start(m_binary, args);
}

void LiveViewCameraRpicam::stop()
{
    m_running = false;
    m_restart.stop();
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.terminate();
        if (!m_proc.waitForFinished(1500))
            m_proc.kill();
    }
}

void LiveViewCameraRpicam::onReadyRead()
{
    m_buffer += m_proc.readAllStandardOutput();

    for (;;) {
        const int start = m_buffer.indexOf(kSoi);
        if (start < 0) {
            m_buffer.clear();
            return;
        }
        const int end = m_buffer.indexOf(kEoi, start + 2);
        if (end < 0) {
            if (start > 0)
                m_buffer.remove(0, start);
            // Schutz gegen unbegrenztes Wachstum bei kaputtem Stream.
            if (m_buffer.size() > 8 * 1024 * 1024)
                m_buffer.clear();
            return;
        }

        const QByteArray frame = m_buffer.mid(start, end + 2 - start);
        m_buffer.remove(0, end + 2);

        QImage img = QImage::fromData(frame, "JPEG");
        if (!img.isNull())
            emit frameReady(img);
    }
}

void LiveViewCameraRpicam::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_running)
        return;
    qWarning() << "rpicam-vid beendet (Code" << exitCode << ", Status" << status
               << "), Neustart in 2s. stderr:"
               << QString::fromUtf8(m_proc.readAllStandardError()).trimmed();
    m_restart.start();
}
