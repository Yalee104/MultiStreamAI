#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include "streamview.h"
#include "Apps/appmanager.h"
#include <QMenu>
#include <QObject>
#include <QCamera>
#include <QCameraFormat>
#include <QMediaCaptureSession>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "mediastreamQt6.h"
#else
#include "mediastream.h"
#endif

class CameraView : public StreamView
{
    Q_OBJECT

public:
    CameraView(QString NewID, QWidget *parent = nullptr, int viewSizeMinW = 160, int viewSizeMinH = 160);
    ~CameraView();

    void        StartCamera(const QCameraDevice &cameraDevice, bool ImageInverted);
    void        StartCamera(const QString CamID,
                            QVideoFrameFormat::PixelFormat PixelFormat,
                            const QSize &FrameSize,
                            float FrameRate,
                            bool ImageInverted);

    void        ConfigCameraView(const QCameraFormat &ViewSettings);
    QString     GetSelectedAppName();
    QString     GetSelectedNetworkName();
    void        SelectApp(QString AppName, QString NetworkName);
    void        changeTargetFPS(int FPS) override;

    QString pixelFormatToString( QVideoFrameFormat::PixelFormat pixelformatvalue );
    const QCameraFormat getCameraViewFinderSettings();

signals:
    Q_INVOKABLE void sendMouseEvent(QMouseEvent * e, QString ID);

public slots:
    void CameraSelectionTrigger(QAction *action);
    void CameraResSelectionTrigger(QAction *action);

    void CameraInvertImage(bool isChecked);
    void DeleteCameraViewRequest();
    void UpdateImageToView(const QImage& frame, const QList<QGraphicsItem*> &overlayItems);

protected:
    void mousePressEvent(QMouseEvent * e);
    void buildViewMenu();
    void ReleaseCamera();
    bool CameraFormatSupported(const QCameraFormat &CameraFormat);

public:
    bool            m_Loop = true;
    bool            m_InvertImage = false;
    QString         m_CameraUniqueDeviceName = nullptr;

protected:
    QMenu           m_ViewMenu;
    QCamera*        m_pCamera = nullptr;
    QMediaCaptureSession* m_captureSession = nullptr;
    MediaStream     m_MediaStream;
    AppManager      m_AppManager;

};

#endif // CAMERAVIEW_H
