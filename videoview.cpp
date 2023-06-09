#include "videoview.h"
#include <QObject>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

VideoView::VideoView(QString NewID, QWidget *parent, int viewSizeMinW, int viewSizeMinH) :
    StreamView(NewID, parent, viewSizeMinW, viewSizeMinH)
{
    setHelperDescription("         Right Click\nTo Select Video Source");

    m_MediaPlayer.setVideoOutput(&m_MediaStream);

    m_AppManager.m_AppID = NewID;

    connect(&m_MediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(PlayerStatusChanged(QMediaPlayer::MediaStatus)));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(&m_MediaPlayer, &QMediaPlayer::errorOccurred, this, &VideoView::PlayerError);
#else
    connect(&m_MediaPlayer, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(PlayerError(QMediaPlayer::Error)));
#endif
    connect(&m_MediaStream, &MediaStream::sendImage, [this](const QImage& frame) {

        if (m_AppManager.AppSelected() == false)
            UpdateImageToView(frame, QList<QGraphicsItem*>());
        else
            m_AppManager.AppImageInfer(frame);
    });

    QObject::connect(&m_AppManager, SIGNAL(sendImage(QImage, QList<QGraphicsItem*>)), this, SLOT(UpdateImageToView(QImage,QList<QGraphicsItem*>)));
}

QString VideoView::GetSelectedAppName()
{
    return m_AppManager.GetSelectedAppName();
}

QString VideoView::GetSelectedNetworkName()
{
    return m_AppManager.GetSelectedNetworkName();
}

void VideoView::changeTargetFPS(int FPS)
{
    m_AppManager.AppLimitFPS(FPS);
    //qDebug() << "Video FPS = " << FPS;
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

#if defined(QT_ON_JETSON) || defined(QT_ON_RK3588)
    //For now we only support few file format h.264
    fileDialog.setNameFilter("*.mp4");
#endif

    if (fileDialog.exec() == QDialog::Accepted) {
        loadSource(fileDialog.selectedUrls().constFirst());
    }
}

void VideoView::ViewMenu_StreamingLinkInput()
{
    bool ok;
    QString address = QInputDialog::getText(this, tr("RTSP"),
                                         tr("Stream Address"), QLineEdit::Normal,
                                         tr("rtsp://"), &ok);
    if (ok && !address.isEmpty()) {
        loadSource(QUrl(address));
        //qDebug() << address;
    }
}

void VideoView::PlayerStatusChanged(QMediaPlayer::MediaStatus Status)
{
    //qDebug() << "PlayerStatusChanged: " << Status;
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void VideoView::PlayerError(QMediaPlayer::Error error, const QString &errorString)
#else
void VideoView::PlayerError(QMediaPlayer::Error error)
#endif
{
    bool bError = true;
    QMessageBox msgBox;

    switch (error) {
    case QMediaPlayer::ResourceError:
    {
        msgBox.setText("Media resource couldn't be resolved");
        break;
    }
    case QMediaPlayer::NetworkError:
    {
        msgBox.setText("A network error occurred.");
        break;
    }
    case QMediaPlayer::AccessDeniedError:
    {
        msgBox.setText("There are not the appropriate permissions to play a media resource.");
        break;
    }

    default:
        bError = false;
        break;
    }

    if (bError == true) {
        msgBox.exec();
        m_MediaPlayer.stop();
    }
}

void VideoView::loadSource(const QUrl &url)
{
    //qDebug() << url.path(QUrl::FullyEncoded);
    //qDebug() << url.scheme();
    //qDebug() << url.authority(QUrl::FullyEncoded);
    //qDebug() << url.host();
    //qDebug() << url.port();
    //qDebug() << url.toString(QUrl::FullyEncoded);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_MediaPlayer.setSource(url);

#ifndef QT_ON_x86
    qDebug() << "WARNING FOR NON X86: On QT6 backend plugin (eg, using gst-pipeline) is NOT longer supported, do expect video decode performance degradation";
    qDebug() << "Suggest to use QT5 instead!";
#endif

#else //QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)


#ifdef QT_ON_JETSON

    QString gstpipe;
    QString scheme = url.scheme();

    if (scheme == tr("file")) {
        //m_MediaPlayer.setMedia(QUrl("gst-pipeline: videotestsrc ! qtvideosink"));
        //QString gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! h264parse ! omxh264dec ! videoconvert ! qtvideosink").arg(url.path(QUrl::FullyEncoded));
        gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! queue ! h264parse ! nvv4l2decoder ! nvvidconv ! qtvideosink").arg(url.path(QUrl::FullyEncoded));
    }
    else if (scheme == tr("rtsp")) {
        //gstpipe = QString("gst-pipeline: rtspsrc location=%1://%2%3 ! rtph264depay ! queue ! h264parse ! nvv4l2decoder ! nvvidconv ! qtvideosink").arg(scheme, url.authority(QUrl::FullyEncoded), url.path(QUrl::FullyEncoded));
        gstpipe = QString("gst-pipeline: rtspsrc location=%1 ! rtph264depay ! queue ! h264parse ! nvv4l2decoder ! nvvidconv ! qtvideosink").arg(url.toString(QUrl::FullyEncoded));
        qDebug() << gstpipe;
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("Unsupported media resource.");
        msgBox.exec();
    }

    m_MediaPlayer.setMedia(QUrl(gstpipe));

#elif QT_ON_RK3588

    QString gstpipe;
    QString scheme = url.scheme();

    if (scheme == tr("file")) {
        //m_MediaPlayer.setMedia(QUrl("gst-pipeline: videotestsrc ! qtvideosink"));
        gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! queue leaky=no max-size-buffers=5 ! h264parse ! mppvideodec ! video/x-raw ! queue leaky=no max-size-buffers=5 ! videoconvert ! queue leaky=no max-size-buffers=5 ! qtvideosink").arg(url.path(QUrl::FullyEncoded));
        //gstpipe = QString("gst-pipeline: filesrc location=/%1 ! qtdemux ! queue ! h264parse ! mppvideodec ! video/x-raw ! videoconvert ! qtvideosink").arg(url.path(QUrl::FullyEncoded));

    }
    else if (scheme == tr("rtsp")) {
        //gstpipe = QString("gst-pipeline: rtspsrc location=%1://%2%3 ! rtph264depay ! queue ! h264parse ! mppvideodec ! videoconvert ! qtvideosink").arg(scheme, url.authority(QUrl::FullyEncoded), url.path(QUrl::FullyEncoded));
        gstpipe = QString("gst-pipeline: rtspsrc location=%1 ! rtph264depay ! queue ! h264parse ! mppvideodec ! videoconvert ! qtvideosink").arg(url.toString(QUrl::FullyEncoded));
        qDebug() << gstpipe;
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("Unsupported media resource.");
        msgBox.exec();
    }

    m_MediaPlayer.setMedia(QUrl(gstpipe));

#else

    m_MediaPlayer.setMedia(url);

#endif

#endif //QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)


    m_SelectedMediaUrl = url;
    m_MediaPlayer.setPlaybackRate(1.0);
}


void VideoView::SelectApp(QString AppName, QString NetworkName)
{
    m_AppManager.LaunchApp(AppName, NetworkName);
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

    m_ViewMenu.addAction("Chose File..", this, SLOT(ViewMenu_ChoseVideoSource()));
    m_ViewMenu.addAction("Chose Stream (RTSP)..", this, SLOT(ViewMenu_StreamingLinkInput()));

    //qDebug() << m_MediaPlayer.mediaStatus();
    if (m_MediaPlayer.mediaStatus() == QMediaPlayer::BufferedMedia) {
        m_ViewMenu.addSeparator();
        QAction* pPlay = m_ViewMenu.addAction("Play", &m_MediaPlayer, SLOT(play()));
        QAction* pPause = m_ViewMenu.addAction("Pause", &m_MediaPlayer, SLOT(pause()));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (m_MediaPlayer.playbackState() == QMediaPlayer::PlayingState)
            pPlay->setEnabled(false);
        else
            pPause->setEnabled(false);
#else
        if (m_MediaPlayer.state() == QMediaPlayer::PlayingState)
            pPlay->setEnabled(false);
        else
            pPause->setEnabled(false);
#endif

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
