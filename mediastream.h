#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class MediaStream : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit MediaStream(QObject *parent = 0);
    ~MediaStream();

    bool Mirror() const;
    void setMirror(bool newMirror);

signals:
    Q_INVOKABLE void sendImageVideoFrame(const QVideoFrame& frame);
    Q_INVOKABLE void sendImagePixmap(const QPixmap& frame);
    Q_INVOKABLE void sendImage(const QImage& frame);

protected:
    bool present(const QVideoFrame &frame) Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    bool start(const QVideoSurfaceFormat &) Q_DECL_OVERRIDE;

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType =
            QAbstractVideoBuffer::NoHandle) const Q_DECL_OVERRIDE;

protected:
    bool m_Mirror = false;
};

#endif // MEDIASTREAM_H
