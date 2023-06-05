#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QMediaDevices>

class MediaStream : public QVideoSink
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

public slots:
    void StreamVideoFrameChanged(const QVideoFrame &frame);

protected:
    bool m_Mirror = false;
};

#endif // MEDIASTREAM_H
