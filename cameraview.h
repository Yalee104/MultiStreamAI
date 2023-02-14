#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "streamview.h"
#include "mediastream.h"
#include "Apps/appmanager.h"
#include <QMenu>
#include <QObject>
#include <QCamera>
#include <QCameraInfo>

class CameraView : public StreamView
{
    Q_OBJECT

public:
    CameraView(QString NewID, QWidget *parent = nullptr, int viewSizeMinW = 160, int viewSizeMinH = 160);
    ~CameraView();

    void        StartCamera(QString CameraDeviceName, bool ImageInverted);
    void        ConfigCameraView(QCameraViewfinderSettings ViewSettings);
    QString     GetSelectedAppName();
    void        SelectApp(QString AppName);
    void        changeTargetFPS(int FPS) override;

    QString pixelFormatToString( QVideoFrame::PixelFormat pixelformatvalue );
    const QCameraViewfinderSettings getCameraViewFinderSettings();

signals:
    Q_INVOKABLE void sendMouseEvent(QMouseEvent * e, QString ID);

public slots:
    void CameraSelectionTrigger(QAction *action);
    void CameraResSelectionTrigger(QAction *action);

    void CameraStatusChanged(QCamera::Status status);
    void CameraInvertImage(bool isChecked);
    void DeleteCameraViewRequest();
    void UpdateImageToView(const QImage& frame, const QList<QGraphicsItem*> &overlayItems);

protected:
    void mousePressEvent(QMouseEvent * e);
    void buildViewMenu();
    void ReleaseCamera();

public:
    bool            m_Loop = true;
    bool            m_InvertImage = false;
    QString         m_CameraUniqueDeviceName = nullptr;

protected:
    QMenu           m_ViewMenu;
    QCamera*        m_pCamera = nullptr;
    MediaStream     m_MediaStream;
    AppManager      m_AppManager;

};

#endif // CAMERAVIEW_H
