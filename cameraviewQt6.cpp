#include "cameraviewQt6.h"
#include <QObject>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>



CameraView::CameraView(QString NewID, QWidget *parent, int viewSizeMinW, int viewSizeMinH) :
    StreamView(NewID, parent, viewSizeMinW, viewSizeMinH)
{
    setHelperDescription("    Right Click\nTo Select Camera");

    m_AppManager.m_AppID = NewID;

    connect(&m_MediaStream, &MediaStream::sendImage, [this](const QImage& frame) {

        if (m_AppManager.AppSelected() == false)
            UpdateImageToView(frame, QList<QGraphicsItem*>());
        else
            m_AppManager.AppImageInfer(frame);
    });

    QObject::connect(&m_AppManager, SIGNAL(sendImage(QImage, QList<QGraphicsItem*>)), this, SLOT(UpdateImageToView(QImage, QList<QGraphicsItem*>)));

}

CameraView::~CameraView()
{
    qDebug() << "~CameraView()";
    ReleaseCamera();
}

QString CameraView::GetSelectedAppName()
{
    return m_AppManager.GetSelectedAppName();
}

QString CameraView::GetSelectedNetworkName()
{
    return m_AppManager.GetSelectedNetworkName();
}

void CameraView::SelectApp(QString AppName, QString NetworkName)
{
    m_AppManager.LaunchApp(AppName, NetworkName);
}


void CameraView::changeTargetFPS(int FPS)
{
    m_AppManager.AppLimitFPS(FPS);
    //qDebug() << "Camera FPS = " << FPS;

}

QString CameraView::pixelFormatToString(QVideoFrameFormat::PixelFormat pixelformatvalue)
{
    switch (pixelformatvalue) {

        //case QVideoFrameFormat::Format_Invalid: return QLatin1String("Invalid");
        case QVideoFrameFormat::Format_ARGB8888: return QLatin1String("ARGB32");
        case QVideoFrameFormat::Format_ARGB8888_Premultiplied: return QLatin1String("ARGB32 PreMul");
        case QVideoFrameFormat::Format_XRGB8888: return QLatin1String("RGB32");
        case QVideoFrameFormat::Format_BGRA8888: return QLatin1String("BGRA32");
        case QVideoFrameFormat::Format_BGRA8888_Premultiplied: return QLatin1String("BGRA32 PreMul");
        case QVideoFrameFormat::Format_XBGR8888: return QLatin1String("BGR32");
        case QVideoFrameFormat::Format_AYUV: return QLatin1String("AYUV444");
        case QVideoFrameFormat::Format_AYUV_Premultiplied: return QLatin1String("AYUV444 PreMul");
        case QVideoFrameFormat::Format_YUV420P: return QLatin1String("YUV420P");
        case QVideoFrameFormat::Format_YV12: return QLatin1String("YV12");
        case QVideoFrameFormat::Format_UYVY: return QLatin1String("UYVY");

        case QVideoFrameFormat::Format_YUYV: return QLatin1String("YUYV");
        case QVideoFrameFormat::Format_NV12: return QLatin1String("NV12");
        case QVideoFrameFormat::Format_NV21: return QLatin1String("NV21");
        //case QVideoFrameFormat::Format_IMC1: return QLatin1String("IMC1");
        //case QVideoFrameFormat::Format_IMC2: return QLatin1String("IMC2");
        //case QVideoFrameFormat::Format_IMC3: return QLatin1String("IMC3");
        //case QVideoFrameFormat::Format_IMC4: return QLatin1String("IMC4");

        //case QVideoFrameFormat::Format_Y8: return QLatin1String("Y8");
        //case QVideoFrameFormat::Format_Y16: return QLatin1String("Y16");
        //case QVideoFrameFormat::Format_Jpeg: return QLatin1String("Jpeg");
        //case QVideoFrameFormat::Format_YUV422P: return QLatin1String("YUV422P");
        default: return QLatin1String("Unknown"); //UNSUPPORTED
    }
}

const QCameraFormat CameraView::getCameraViewFinderSettings()
{
    if (m_pCamera == nullptr)
        return QCameraFormat();

    return m_pCamera->cameraFormat();
}

void CameraView::UpdateImageToView(const QImage &frame, const QList<QGraphicsItem*> &overlayItems)
{
    if (overlayItems.size())
        qDebug() << "CameraView::UpdateImageToView QGraphicsItem overlay reserved for future use only";

    drawStreamFramePixmap(QPixmap::fromImage(frame));
}


void CameraView::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::RightButton) {
        buildViewMenu();
        m_ViewMenu.popup(e->globalPos());
    }
}

bool CameraView::CameraFormatSupported(const QCameraFormat &CameraFormat)
{
    //Filter out the format we do not support
    if (pixelFormatToString(CameraFormat.pixelFormat()) == "Unknown")
        return false;

    //Filter out size that is greater than 1920 as performance won't be good and not necessary for such high resolution
    if (CameraFormat.resolution().rwidth() > 1920)
        return false;

    //Filter out frame rate more than 30, we don't really need more than that
    if (CameraFormat.maxFrameRate() > 30)
        return false;

    return true;
}


void CameraView::StartCamera(const QCameraDevice &cameraDevice, bool ImageInverted)
{
    ReleaseCamera();

    m_pCamera = new QCamera(cameraDevice);

    QCameraFormat CamDefaultFormat;
    for (const QCameraFormat &setting : cameraDevice.videoFormats()) {

        if (!CameraFormatSupported(setting))
            continue;

        //Set the default one, the one with highest resolution
        if (setting.resolution().rwidth() > CamDefaultFormat.resolution().rwidth()) {
            CamDefaultFormat = setting;
        }
    }

    m_pCamera->setCameraFormat(CamDefaultFormat);

    m_CameraUniqueDeviceName = QString::fromStdString(cameraDevice.id().toStdString());
    m_MediaStream.setMirror(ImageInverted);

    m_captureSession = new QMediaCaptureSession();
    m_captureSession->setCamera(m_pCamera);
    m_captureSession->setVideoSink(&m_MediaStream);
    m_pCamera->start();

    QCameraFormat ViewSettings = m_pCamera->cameraFormat();
    qDebug() << "Selected Resolution: " << ViewSettings.resolution() << " FPS: " << ViewSettings.maxFrameRate() << "Pixel Format: " << ViewSettings.pixelFormat();

}

void CameraView::StartCamera(   const QString CamID,
                                QVideoFrameFormat::PixelFormat PixelFormat,
                                const QSize &FrameSize,
                                float FrameRate,
                                bool ImageInverted)
{
    ReleaseCamera();

    const QCameraDevice *pSelectedCamDevice = nullptr;
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {

        if (CamID.compare(QString::fromStdString(cameraDevice.id().toStdString())) == 0) {
            pSelectedCamDevice = &cameraDevice;
            break;
        }
    }

    QCameraFormat SelCamFormat;
    for (const QCameraFormat &setting : pSelectedCamDevice->videoFormats()) {

        if (!CameraFormatSupported(setting))
            continue;

        if ((setting.resolution().width() == FrameSize.width()) &&
            (setting.resolution().height() == FrameSize.height()) &&
            (setting.maxFrameRate() == FrameRate) &&
            (setting.pixelFormat() == PixelFormat))
        {
            SelCamFormat = setting;
        }
    }


    m_pCamera = new QCamera(*pSelectedCamDevice);

    m_pCamera->setCameraFormat(SelCamFormat);

    m_CameraUniqueDeviceName = CamID;
    m_MediaStream.setMirror(ImageInverted);

    m_captureSession = new QMediaCaptureSession();
    m_captureSession->setCamera(m_pCamera);
    m_captureSession->setVideoSink(&m_MediaStream);
    m_pCamera->start();

    QCameraFormat ViewSettings = m_pCamera->cameraFormat();
    qDebug() << "Selected Resolution: " << ViewSettings.resolution() << " FPS: " << ViewSettings.maxFrameRate() << "Pixel Format: " << ViewSettings.pixelFormat();

}


void CameraView::CameraSelectionTrigger(QAction *action)
{
    StartCamera(qvariant_cast<QCameraDevice>(action->data()), true);
}

void CameraView::ConfigCameraView(const QCameraFormat &ViewSettings)
{
    m_pCamera->setCameraFormat(ViewSettings);

    //Ugly WA for Windows for the change resolution to take effect,
    //On Ubuntu 20.04 tested we don't need stop/start, just setCameraFormat will take effect
    //On Embedded not tested yet
#ifdef _WIN32
    m_pCamera->stop();
    m_pCamera->start();
#endif

    qDebug() << "Selected Resolution: " << ViewSettings.resolution() << " FPS: " << ViewSettings.maxFrameRate() << "Pixel Format: " << ViewSettings.pixelFormat();

}

void CameraView::CameraResSelectionTrigger(QAction *action)
{
    ConfigCameraView(qvariant_cast<QCameraFormat>(action->data()));
}

void CameraView::CameraInvertImage(bool isChecked)
{
    m_MediaStream.setMirror(isChecked);
    m_InvertImage = isChecked;
}


void CameraView::buildViewMenu()
{
    m_ViewMenu.clear();

    //Build Available Camera List
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QMenu* CameraSelSubMenu = m_ViewMenu.addMenu("Camera Selection");
    for (const QCameraDevice &cameraDevice : cameras) {
        QAction *pCameraSelAction = CameraSelSubMenu->addAction(cameraDevice.description());
        pCameraSelAction->setData(QVariant::fromValue(cameraDevice));

        /*
        QByteArray CamID = cameraDevice.id();
        qDebug() << "Camera ID Byte Total: " << CamID.size() << ", " << CamID.isValidUtf8();
        qDebug() << "Camera ID string: " << QString::fromStdString(CamID.toStdString());
        */

        if (m_pCamera) {
            if (!m_CameraUniqueDeviceName.compare(QString::fromStdString(cameraDevice.id().toStdString())))
                pCameraSelAction->setEnabled(false);
        }

    }

    connect(CameraSelSubMenu, SIGNAL(triggered(QAction*)), this, SLOT(CameraSelectionTrigger(QAction*)));

    //Build camera view control
    if (m_pCamera) {
        m_ViewMenu.addSeparator();

        //Start/Stop
        QAction *pStartAction = m_ViewMenu.addAction("Start", m_pCamera, SLOT(start()));
        QAction *pStopAction = m_ViewMenu.addAction("Stop", m_pCamera, SLOT(stop()));

        //Allow User to invert camera image as on some camera it appear upside down
        QAction *pInvertImage = m_ViewMenu.addAction("Invert Image", this, SLOT(CameraInvertImage(bool)));
        pInvertImage->setCheckable(true);
        m_InvertImage = m_MediaStream.Mirror();
        pInvertImage->setChecked(m_InvertImage);

        //Enable/Disable item as necessary base on curent camera state
        if (m_pCamera->isActive()) {
            pStartAction->setEnabled(false);
            pInvertImage->setEnabled(true);
        }
        else {
            pStopAction->setEnabled(false);
            pInvertImage->setEnabled(false);
        }

        //Add available camera resolution sub menu selection
        m_ViewMenu.addSeparator();
        QMenu* pCameraResSelSubMenu = m_ViewMenu.addMenu("Camera Resolution");
        for (const QCameraFormat &setting : m_pCamera->cameraDevice().videoFormats()) {

            if (!CameraFormatSupported(setting))
                continue;

            QString ResolutionFormat = QString("%1x%2 @%3 [%4]").arg(QString::number(setting.resolution().rwidth()),
                                                               QString::number(setting.resolution().rheight()),
                                                               QString::number(setting.maxFrameRate()),
                                                               pixelFormatToString(setting.pixelFormat()));

            QAction *pCameraResSelAction = pCameraResSelSubMenu->addAction(ResolutionFormat);
            QVariant v = QVariant::fromValue(setting);
            pCameraResSelAction->setData(v);

        }
        connect(pCameraResSelSubMenu, SIGNAL(triggered(QAction*)), this, SLOT(CameraResSelectionTrigger(QAction*)));

        //Add available App selection submenu from AppManager
        m_ViewMenu.addSeparator();
        m_ViewMenu.addMenu(m_AppManager.getAppMenu());
    }

    //Build View Control
    m_ViewMenu.addSeparator();
    m_ViewMenu.addAction("Delete", this, SLOT(DeleteCameraViewRequest()));
}

void CameraView::ReleaseCamera()
{
    if (m_pCamera) {
        if (m_pCamera->isActive())
            m_pCamera->stop();
        m_pCamera->deleteLater();
        m_pCamera = nullptr;
        m_CameraUniqueDeviceName.clear();
    }

    if (m_captureSession) {
        m_captureSession->deleteLater();
        m_captureSession = nullptr;
    }
}

void CameraView::DeleteCameraViewRequest()
{
    emit stremViewWantsDeleteEvent(this->ID);
}

