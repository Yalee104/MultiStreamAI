#include "mediastream.h"
#include <QDebug>


MediaStream::MediaStream(QObject *parent) : QAbstractVideoSurface(parent)
{
    //N/A
}


MediaStream::~MediaStream()
{
    qDebug() << "~MediaStream";
}

bool MediaStream::start(const QVideoSurfaceFormat &videoformat)
{
    qDebug() << QVideoFrame::imageFormatFromPixelFormat(videoformat.pixelFormat());//The format is RGB32
    if(QVideoFrame::imageFormatFromPixelFormat(videoformat.pixelFormat()) != QImage::Format_Invalid && !videoformat.frameSize().isEmpty()){
        QAbstractVideoSurface::start(videoformat);

        return true;
    }
    return false;
}

void MediaStream::stop()
{
    QAbstractVideoSurface::stop();
}

QList<QVideoFrame::PixelFormat> MediaStream::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        //qDebug() << "VideoSurface NoHandle supportedPixelFormats" << (void*)this;
        QList<QVideoFrame::PixelFormat> listPixelFormats;

        listPixelFormats << QVideoFrame::Format_RGB24
        << QVideoFrame::Format_ARGB32_Premultiplied
        << QVideoFrame::Format_RGB32
        << QVideoFrame::Format_RGB565
        << QVideoFrame::Format_RGB555
        << QVideoFrame::Format_ARGB8565_Premultiplied
        << QVideoFrame::Format_BGRA32
        << QVideoFrame::Format_BGRA32_Premultiplied
        << QVideoFrame::Format_BGR32
        << QVideoFrame::Format_BGR24
        << QVideoFrame::Format_BGR565
        << QVideoFrame::Format_BGR555;

        // Return the formats you will support
        return listPixelFormats;
    }
    else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool MediaStream::Mirror() const
{
    return m_Mirror;
}

void MediaStream::setMirror(bool newMirror)
{
    m_Mirror = newMirror;
}


bool MediaStream::present(const QVideoFrame &frame)
{
    //qDebug() << "VideoSurfaces present: " << frame.pixelFormat();
    if (frame.isValid())
    {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
        QImage* image = new QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(),
                                   QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));

        //convert to RGB888 which is what we will need for inference
        //TODO: is there a better or faster way by obtaining RGB888 directly from QVideoFrame?
        QImage image888_rgb = image->convertToFormat(QImage::Format_RGB888);
        delete image;
        cloneFrame.unmap();

        //Camera on Windows the image is upside down, we need to flip it
        //NOTE: Need to test on linux because it might not be needed.
        if (m_Mirror)
            emit sendImage(image888_rgb.mirrored());
        else
            emit sendImage(image888_rgb);

        return true;
    }

    stop();
    return false;
}
