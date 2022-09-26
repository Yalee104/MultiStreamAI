#include "videoview.h"
#include <QObject>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

VideoView::VideoView(QString NewID, QWidget *parent, int viewSizeMinW, int viewSizeMinH) :
    StreamView(NewID, parent, viewSizeMinW, viewSizeMinH)
{
    setHelperDescription("         Right Click\nTo Select Video Source");

    m_MediaPlayer.setVideoOutput(&m_MediaStream);

    connect(&m_MediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(PlayerStatusChanged(QMediaPlayer::MediaStatus)));

    connect(&m_MediaStream, &MediaStream::sendImage, [this](const QImage& frame) {

        if (m_AppManager.AppSelected() == false)
            UpdateImageToView(frame, QList<QGraphicsItem*>());
        else
            m_AppManager.AppImageInfer(frame);
    });

    QObject::connect(&m_AppManager, SIGNAL(sendImage(QImage, QList<QGraphicsItem*>)), this, SLOT(UpdateImageToView(QImage,QList<QGraphicsItem*>)));
}

void VideoView::UpdateImageToView(const QImage &frame, const QList<QGraphicsItem*> &overlayItems)
{
    if (overlayItems.size())
        qDebug() << "VideoView::UpdateImageToView QGraphicsItem overlay reserved for future use only";

    drawStreamFramePixmap(QPixmap::fromImage(frame));
}


void VideoView::ViewMenu_ChoseVideoSource()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Movie"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));

#ifdef QT_ON_JETSON
    //For now we only support few file format h.264
    fileDialog.setNameFilter("*.mp4");
#endif

    if (fileDialog.exec() == QDialog::Accepted) {
        loadSource(fileDialog.selectedUrls().constFirst());
    }
}


void VideoView::PlayerStatusChanged(QMediaPlayer::MediaStatus Status)
{
    switch (Status) {

        case QMediaPlayer::LoadedMedia:
            m_MediaPlayer.play();
            break;

        case QMediaPlayer::EndOfMedia:
            if (m_Loop)
                m_MediaPlayer.play();
            break;

        default:
            break;
    }
}

void VideoView::loadSource(const QUrl &url)
{
    //qDebug() << url.path(QUrl::FullyEncoded);

#ifdef QT_ON_JETSON
    //m_MediaPlayer.setMedia(QUrl("gst-pipeline: videotestsrc ! qtvideosink"));
    //QString gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! h264parse ! omxh264dec ! videoconvert ! qtvideosink").arg(url.path(QUrl::FullyEncoded));
    QString gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! queue ! h264parse ! nvv4l2decoder ! nvvidconv ! qtvideosink").arg(url.path(QUrl::FullyEncoded));
    m_MediaPlayer.setMedia(QUrl(gstpipe));
#else
    m_MediaPlayer.setMedia(url);
#endif

    m_MediaPlayer.setPlaybackRate(1.0);
}

void VideoView::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::RightButton) {
        buildViewMenu();
        m_ViewMenu.popup(e->globalPos());
    }
}

void VideoView::buildViewMenu()
{
    m_ViewMenu.clear();

    m_ViewMenu.addAction("Chose Source..", this, SLOT(ViewMenu_ChoseVideoSource()));

    if (m_MediaPlayer.isVideoAvailable()) {
        m_ViewMenu.addSeparator();
        QAction* pPlay = m_ViewMenu.addAction("Play", &m_MediaPlayer, SLOT(play()));
        QAction* pPause = m_ViewMenu.addAction("Pause", &m_MediaPlayer, SLOT(pause()));

        if (m_MediaPlayer.state() == QMediaPlayer::PlayingState)
            pPlay->setEnabled(false);

        if (m_MediaPlayer.state() != QMediaPlayer::PlayingState)
            pPause->setEnabled(false);

        //Add available App selection submenu from AppManager
        m_ViewMenu.addSeparator();
        m_ViewMenu.addMenu(m_AppManager.getAppMenu());
    }

    //Build View Control
    m_ViewMenu.addSeparator();
    m_ViewMenu.addAction("Delete", this, SLOT(DeleteVideoViewRequest()));

}

void VideoView::DeleteVideoViewRequest()
{
    emit stremViewWantsDeleteEvent(this->ID);
}
