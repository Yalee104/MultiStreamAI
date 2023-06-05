#include "mediastreamQt6.h"
#include <QDebug>


MediaStream::MediaStream(QObject *parent) : QVideoSink(parent)
{
    connect(this, &QVideoSink::videoFrameChanged,
            this, &MediaStream::StreamVideoFrameChanged);

}


MediaStream::~MediaStream()
{
    qDebug() << "~MediaStream";
}

bool MediaStream::Mirror() const
{
    return m_Mirror;
}

void MediaStream::setMirror(bool newMirror)
{
    m_Mirror = newMirror;
}


void MediaStream::StreamVideoFrameChanged(const QVideoFrame &frame)
{
    //qDebug() << "VideoSurfaces present: " << frame.pixelFormat();
    if (frame.isValid())
    {
        QImage image888_rgb = frame.toImage().convertToFormat(QImage::Format_RGB888);

        //Camera on Windows the image is upside down, we need to flip it
        //NOTE: Need to test on linux because it might not be needed.
        if (m_Mirror)
            emit sendImage(image888_rgb.mirrored());
        else
            emit sendImage(image888_rgb);

    }

    return;
}
