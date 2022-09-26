#include "cameraview.h"
#include <QObject>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>



CameraView::CameraView(QString NewID, QWidget *parent, int viewSizeMinW, int viewSizeMinH) :
    StreamView(NewID, parent, viewSizeMinW, viewSizeMinH)
{
    setHelperDescription("    Right Click\nTo Select Camera");

    QObject::connect(&m_MediaStream, &MediaStream::sendImage, [this](const QImage& frame) {

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

QString CameraView::pixelFormatToString(QVideoFrame::PixelFormat pixelformatvalue)
{
    switch (pixelformatvalue) {

        //case QVideoFrame::Format_Invalid: return QLatin1String("Invalid");
        case QVideoFrame::Format_ARGB32: return QLatin1String("ARGB32");
        case QVideoFrame::Format_ARGB32_Premultiplied: return QLatin1String("ARGB32 PreMul");
        case QVideoFrame::Format_RGB32: return QLatin1String("RGB32");
        case QVideoFrame::Format_RGB24: return QLatin1String("RGB24");
        case QVideoFrame::Format_RGB565: return QLatin1String("RGB565");
        case QVideoFrame::Format_RGB555: return QLatin1String("RGB555");
        case QVideoFrame::Format_ARGB8565_Premultiplied: return QLatin1String("RGB565 PreMUl");
        case QVideoFrame::Format_BGRA32: return QLatin1String("BGRA32");
        case QVideoFrame::Format_BGRA32_Premultiplied: return QLatin1String("BGRA32 PreMul");
        case QVideoFrame::Format_BGR32: return QLatin1String("BGR32");
        case QVideoFrame::Format_BGR24: return QLatin1String("BGR24");
        case QVideoFrame::Format_BGR565: return QLatin1String("BGR565");
        case QVideoFrame::Format_BGR555: return QLatin1String("BGR555");
        case QVideoFrame::Format_BGRA5658_Premultiplied: return QLatin1String("BGRA5658 PreMul");
        case QVideoFrame::Format_AYUV444: return QLatin1String("AYUV444");
        case QVideoFrame::Format_AYUV444_Premultiplied: return QLatin1String("AYUV444 PreMul");
        case QVideoFrame::Format_YUV444: return QLatin1String("YUV444");
        case QVideoFrame::Format_YUV420P: return QLatin1String("YUV420P");
        case QVideoFrame::Format_YV12: return QLatin1String("YV12");
        case QVideoFrame::Format_UYVY: return QLatin1String("UYVY");

        case QVideoFrame::Format_YUYV: return QLatin1String("YUYV");
        //case QVideoFrame::Format_NV12: return QLatin1String("NV12");
        //case QVideoFrame::Format_NV21: return QLatin1String("NV21");
        //case QVideoFrame::Format_IMC1: return QLatin1String("IMC1");
        //case QVideoFrame::Format_IMC2: return QLatin1String("IMC2");
        //case QVideoFrame::Format_IMC3: return QLatin1String("IMC3");
        //case QVideoFrame::Format_IMC4: return QLatin1String("IMC4");

        //case QVideoFrame::Format_Y8: return QLatin1String("Y8");
        //case QVideoFrame::Format_Y16: return QLatin1String("Y16");
        case QVideoFrame::Format_Jpeg: return QLatin1String("Jpeg");
        //case QVideoFrame::Format_CameraRaw: return QLatin1String("CameraRaw");
        //case QVideoFrame::Format_AdobeDng: return QLatin1String("AdobeDng");
        //case QVideoFrame::Format_ABGR32: return QLatin1String("ABGR32");
        //case QVideoFrame::Format_YUV422P: return QLatin1String("YUV422P");
        default: return QLatin1String("Unknown"); //UNSUPPORTED
    }
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


void CameraView::CameraSelectionTrigger(QAction *action)
{

    ReleaseCamera();

    m_pCamera = new QCamera(action->data().toString().toUtf8());

    m_MediaStream.setMirror(true);
    m_pCamera->setViewfinder(&m_MediaStream);
    m_CameraUniqueDeviceName = action->data().toString();
    connect(m_pCamera, SIGNAL(statusChanged(QCamera::Status)), this, SLOT(CameraStatusChanged(QCamera::Status)));
    m_pCamera->start();

}

void CameraView::CameraResSelectionTrigger(QAction *action)
{
    QVariant v = action->data();
    QCameraViewfinderSettings mysetting = (QCameraViewfinderSettings) v.value<QCameraViewfinderSettings>();

    m_pCamera->setViewfinderSettings(mysetting);

    qDebug() << "My Resolution: " << mysetting.resolution() << " FPS: " << mysetting.maximumFrameRate() << "Pixel Format: " << mysetting.pixelFormat();
}

void CameraView::CameraStatusChanged(QCamera::Status status)
{
    //qDebug() << status;
    if ((status == QCamera::UnloadedStatus) || (status == QCamera::UnavailableStatus)) {
        ReleaseCamera();
    }
    else {
        if (status == QCamera::LoadedStatus)
            m_pCamera->supportedViewfinderSettings();
    }
}

void CameraView::CameraInvertImage(bool isChecked)
{
    m_MediaStream.setMirror(isChecked);
}


void CameraView::buildViewMenu()
{
    m_ViewMenu.clear();

    //Build Available Camera List
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    QMenu* CameraSelSubMenu = m_ViewMenu.addMenu("Camera Selection");
    for (const QCameraInfo &cameraInfo : cameras) {
        QAction *pCameraSelAction = CameraSelSubMenu->addAction(cameraInfo.description());
        pCameraSelAction->setData(cameraInfo.deviceName());

        if (m_pCamera) {
            if (!m_CameraUniqueDeviceName.compare(cameraInfo.deviceName()))
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
        pInvertImage->setChecked(m_MediaStream.Mirror());

        //Enable/Disable item as necessary base on curent camera state
        QCamera::State CurrentState = m_pCamera->state();
        if (CurrentState == QCamera::ActiveState) {
            pStartAction->setEnabled(false);
            pInvertImage->setEnabled(true);
        }
        if (CurrentState != QCamera::ActiveState) {
            pStopAction->setEnabled(false);
            pInvertImage->setEnabled(false);
        }

        //Add available camera resolution sub menu selection
        m_ViewMenu.addSeparator();
        QMenu* pCameraResSelSubMenu = m_ViewMenu.addMenu("Camera Resolution");
        for (const QCameraViewfinderSettings &setting : m_pCamera->supportedViewfinderSettings()) {

            //Filter out the format we do not support
            if (pixelFormatToString(setting.pixelFormat()) == "Unknown")
                continue;

            QString ResolutionFormat = QString("%1x%2 @%3 [%4]").arg(QString::number(setting.resolution().rwidth()),
                                                               QString::number(setting.resolution().rheight()),
                                                               QString::number(setting.maximumFrameRate()),
                                                               pixelFormatToString(setting.pixelFormat()));

            QAction *pCameraResSelAction = pCameraResSelSubMenu->addAction(ResolutionFormat);
            QVariant v = QVariant::fromValue((QCameraViewfinderSettings) setting);
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
        if (m_pCamera->status() == QCamera::ActiveStatus)
            m_pCamera->stop();
        m_pCamera->deleteLater();
        m_pCamera = nullptr;
        m_CameraUniqueDeviceName.clear();
    }
}

void CameraView::DeleteCameraViewRequest()
{
    emit stremViewWantsDeleteEvent(this->ID);
}

