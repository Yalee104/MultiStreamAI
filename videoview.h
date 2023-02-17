#ifndef VIDEOVIEW_H
#define VIDEOVIEW_H

#include "Apps/appmanager.h"
#include "streamview.h"
#include "mediastream.h"
#include <QMenu>
#include <QObject>
#include <QMediaPlayer>

class VideoView : public StreamView
{
    Q_OBJECT
public:
    VideoView(QString NewID, QWidget *parent = nullptr, int viewSizeMinW = 160, int viewSizeMinH = 160);
    void loadSource(const QUrl &url);

    void        SelectApp(QString AppName, QString NetworkName);
    QString     GetSelectedNetworkName();
    QString     GetSelectedAppName();
    void        changeTargetFPS(int FPS) override;

signals:
    Q_INVOKABLE void sendMouseEvent(QMouseEvent * e, QString ID);

public slots:
    void ViewMenu_ChoseVideoSource();
    void ViewMenu_StreamingLinkInput();
    void PlayerStatusChanged(QMediaPlayer::MediaStatus state);
    void PlayerError(QMediaPlayer::Error error);
    void DeleteVideoViewRequest();
    void UpdateImageToView(const QImage& frame, const QList<QGraphicsItem*> &overlayItems);

protected:
    void mousePressEvent(QMouseEvent * e);
    void buildViewMenu();

public:
    bool            m_Loop = true;
    QUrl            m_SelectedMediaUrl;

protected:
    QMenu           m_ViewMenu;
    QMediaPlayer    m_MediaPlayer;
    MediaStream     m_MediaStream;
    AppManager      m_AppManager;
};

#endif // VIDEOVIEW_H
