#include "streamcontainer.h"
#include <QMenu>
#include <QDebug>
#include <QFile>
#include "videoview.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "cameraviewQt6.h"
#else
#include "cameraview.h"
#endif

#define STREAM_JSON_KEY_STREAM_DESCRIPTOR       "StreamDescriptor"
#define STREAM_JSON_KEY_TYPE                    "Type"
#define STREAM_JSON_KEY_TYPE_VALUE_VIDEO        "Video"
#define STREAM_JSON_KEY_TYPE_VALUE_CAMERA       "Camera"

#define STREAN_JSON_KEY_VIDEO_URL               "Url"

#define STREAM_JSON_KEY_CAMERA_DEVICE_ID            "CameraID"
#define STREAM_JSON_KEY_CAMERA_VIEW_PIXEL_FORMAT    "ViewSetting_PixelFormat"
#define STREAM_JSON_KEY_CAMERA_VIEW_WIDTH           "ViewSetting_Width"
#define STREAM_JSON_KEY_CAMERA_VIEW_HEIGHT          "ViewSetting_Height"
#define STREAM_JSON_KEY_CAMERA_VIEW_MAX_FPS         "ViewSetting_MaxFPS"
#define STREAM_JSON_KEY_CAMERA_VIEW_INVERT_IMAGE    "InvertImage"

#define STREAM_JSON_KEY_APP_NAME_SELECTED           "SelectedAppName"
#define STREAM_JSON_KEY_APP_NETWORK_SELECTED        "SelectedAppNetwork"

StreamContainer::StreamContainer(QObject *parent)
    : QObject{parent}
{

}

StreamContainer::~StreamContainer()
{
    while (!StreamList.isEmpty())
        delete StreamList.takeFirst();
}

void StreamContainer::ConfigStream(StreamView* pStreamView, QJsonValue StreamJsonConfigValue)
{

    QJsonObject ConfigInfo = StreamJsonConfigValue.toObject();

    if (dynamic_cast<const VideoView*>(pStreamView) != nullptr) {
        VideoView* pVideoView = dynamic_cast<VideoView*>(pStreamView);

        QJsonObject VideoJsonObj;

        QString VideoUrl = ConfigInfo[STREAN_JSON_KEY_VIDEO_URL].toString();
        QString SelectedAppName = ConfigInfo[STREAM_JSON_KEY_APP_NAME_SELECTED].toString();
        QString SelectedNetworkName = ConfigInfo[STREAM_JSON_KEY_APP_NETWORK_SELECTED].toString();

        pVideoView->loadSource(QUrl(VideoUrl));
        pVideoView->SelectApp(SelectedAppName, SelectedNetworkName);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

    if (dynamic_cast<const CameraView*>(pStreamView) != nullptr) {

        CameraView* pCameraView = dynamic_cast<CameraView*>(pStreamView);

        QString CameraDeviceID = ConfigInfo[STREAM_JSON_KEY_CAMERA_DEVICE_ID].toString();
        int     CameraPixelFormat = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_PIXEL_FORMAT].toInt();
        int     CameraView_Width = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_WIDTH].toInt();
        int     CameraView_Heights = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_HEIGHT].toInt();
        float   CameraView_MaxFPS = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_MAX_FPS].toDouble();
        bool    CameraView_InvertImage = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_INVERT_IMAGE].toBool();

        QVideoFrameFormat::PixelFormat ePixelFormat = static_cast<QVideoFrameFormat::PixelFormat>(CameraPixelFormat);

        pCameraView->StartCamera(   CameraDeviceID,
                                    ePixelFormat,
                                    QSize(CameraView_Width,CameraView_Heights),
                                    CameraView_MaxFPS,
                                    CameraView_InvertImage);


        QString SelectedAppName = ConfigInfo[STREAM_JSON_KEY_APP_NAME_SELECTED].toString();
        QString SelectedNetworkName = ConfigInfo[STREAM_JSON_KEY_APP_NETWORK_SELECTED].toString();

        pCameraView->SelectApp(SelectedAppName, SelectedNetworkName);
    }

#else  //TODO

    if (dynamic_cast<const CameraView*>(pStreamView) != nullptr) {

        CameraView* pCameraView = dynamic_cast<CameraView*>(pStreamView);

        QString CameraDevice = ConfigInfo[STREAM_JSON_KEY_CAMERA_DEVICE_ID].toString();
        int     CameraPixelFormat = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_PIXEL_FORMAT].toInt();
        int     CameraView_Width = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_WIDTH].toInt();
        int     CameraView_Heights = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_HEIGHT].toInt();
        float   CameraView_MaxFPS = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_MAX_FPS].toDouble();
        bool    CameraView_InvertImage = ConfigInfo[STREAM_JSON_KEY_CAMERA_VIEW_INVERT_IMAGE].toBool();

        QVideoFrame::PixelFormat ePixelFormat = static_cast<QVideoFrame::PixelFormat>(CameraPixelFormat);
        QCameraViewfinderSettings CameraViewSetting;
        CameraViewSetting.setMaximumFrameRate(CameraView_MaxFPS);
        CameraViewSetting.setPixelFormat(ePixelFormat);
        CameraViewSetting.setResolution(CameraView_Width, CameraView_Heights);

        pCameraView->StartCamera(CameraDevice, CameraView_InvertImage);
        pCameraView->ConfigCameraView(CameraViewSetting);

        QString SelectedAppName = ConfigInfo[STREAM_JSON_KEY_APP_NAME_SELECTED].toString();
        QString SelectedNetworkName = ConfigInfo[STREAM_JSON_KEY_APP_NETWORK_SELECTED].toString();

        pCameraView->SelectApp(SelectedAppName, SelectedNetworkName);
    }
#endif

}


QJsonArray StreamContainer::GetStreamDescriptorListFromJson(QJsonObject LoadedConfigJsonObj)
{
    QJsonArray StreamDescriptorList;

    if (LoadedConfigJsonObj.contains(STREAM_JSON_KEY_STREAM_DESCRIPTOR) && LoadedConfigJsonObj[STREAM_JSON_KEY_STREAM_DESCRIPTOR].isArray()) {

        StreamDescriptorList = LoadedConfigJsonObj[STREAM_JSON_KEY_STREAM_DESCRIPTOR].toArray();
    }

    return StreamDescriptorList;
}


eStreamViewType StreamContainer::GetStreamViewType(QJsonValue StreamJsonValue)
{
    eStreamViewType StreamType = eStreamViewType::UNKNOWN;

    QJsonObject StreamObj = StreamJsonValue.toObject();

    if (StreamObj[STREAM_JSON_KEY_TYPE].toString().compare(STREAM_JSON_KEY_TYPE_VALUE_VIDEO) == 0) {
        StreamType = eStreamViewType::VIDEO;
    }

    if (StreamObj[STREAM_JSON_KEY_TYPE].toString().compare(STREAM_JSON_KEY_TYPE_VALUE_CAMERA) == 0) {
        StreamType = eStreamViewType::CAMERA;
    }

    return StreamType;
}


void StreamContainer::SaveStreamInfoToFile(QString FileName)
{
    QJsonObject StreamInfo;
    QJsonArray  StreamDescriptor;

    for (int i = 0; i < StreamList.size(); ++i) {

        StreamView *pGetView = StreamList.at(i);
        if (dynamic_cast<const CameraView*>(pGetView) != nullptr) {
            CameraView* pCameraView = dynamic_cast<CameraView*>(pGetView);
            QJsonObject CameraJsonObj;

            if (pCameraView->m_CameraUniqueDeviceName != nullptr) {
                CameraJsonObj[STREAM_JSON_KEY_TYPE] = STREAM_JSON_KEY_TYPE_VALUE_CAMERA;

                CameraJsonObj[STREAM_JSON_KEY_CAMERA_DEVICE_ID] = pCameraView->m_CameraUniqueDeviceName;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QCameraFormat CameraViewSetting = pCameraView->getCameraViewFinderSettings();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_PIXEL_FORMAT] = CameraViewSetting.pixelFormat();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_WIDTH] = CameraViewSetting.resolution().rwidth();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_HEIGHT] = CameraViewSetting.resolution().rheight();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_MAX_FPS] = CameraViewSetting.maxFrameRate();
#else
                const QCameraViewfinderSettings CameraViewSetting = pCameraView->getCameraViewFinderSettings();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_PIXEL_FORMAT] = CameraViewSetting.pixelFormat();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_WIDTH] = CameraViewSetting.resolution().rwidth();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_HEIGHT] = CameraViewSetting.resolution().rheight();
                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_MAX_FPS] = CameraViewSetting.maximumFrameRate();
#endif

                CameraJsonObj[STREAM_JSON_KEY_CAMERA_VIEW_INVERT_IMAGE] = pCameraView->m_InvertImage;

                CameraJsonObj[STREAM_JSON_KEY_APP_NAME_SELECTED] = pCameraView->GetSelectedAppName();
                CameraJsonObj[STREAM_JSON_KEY_APP_NETWORK_SELECTED] = pCameraView->GetSelectedNetworkName();

                StreamDescriptor.append(CameraJsonObj);
            }
        }

        if (dynamic_cast<const VideoView*>(pGetView) != nullptr) {
            VideoView* pVideoView = dynamic_cast<VideoView*>(pGetView);
            QJsonObject VideoJsonObj;

            QString VideoUrl = pVideoView->m_SelectedMediaUrl.url();
            if (VideoUrl.size()) {
                VideoJsonObj[STREAM_JSON_KEY_TYPE] = STREAM_JSON_KEY_TYPE_VALUE_VIDEO;
                VideoJsonObj[STREAN_JSON_KEY_VIDEO_URL] = VideoUrl;
                VideoJsonObj[STREAM_JSON_KEY_APP_NAME_SELECTED] = pVideoView->GetSelectedAppName();
                VideoJsonObj[STREAM_JSON_KEY_APP_NETWORK_SELECTED] = pVideoView->GetSelectedNetworkName();

                StreamDescriptor.append(VideoJsonObj);
            }
        }

    }

    if (StreamDescriptor.count()) {
        StreamInfo[STREAM_JSON_KEY_STREAM_DESCRIPTOR] = StreamDescriptor;

        QFile saveFile(FileName);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning("Couldn't open save file.");
        }
        saveFile.write(QJsonDocument(StreamInfo).toJson());
    }
}


StreamView* StreamContainer::CreateNewStream(QString NewID, eStreamViewType StreamType)
{
    StreamView* pNewStreamView = nullptr;
    if (StreamType == eStreamViewType::CAMERA) {
        pNewStreamView = new CameraView(NewID);
    }
    else {
        pNewStreamView = new VideoView(NewID);
    }

    StreamList.push_back(pNewStreamView);

    return pNewStreamView;
}

void StreamContainer::DeleteStream(QString ID)
{
    for (int i = 0; i < StreamList.size(); ++i) {

        if (StreamList.at(i)->ID == ID) {
           StreamView *pGetView = StreamList.at(i);
           StreamList.removeAt(i);
           pGetView->disconnect();
           pGetView->deleteLater();

           break;
        }
    }
}

QList<QString>  StreamContainer::GetAllStreamViewID()
{
    QList<QString> IdList;

    for (int i = 0; i < StreamList.size(); ++i) {
        IdList.push_back(StreamList.at(i)->ID);
    }

    return IdList;
}

StreamView *StreamContainer::GetStreamViewByID(QString ID)
{
    StreamView *pGetView = nullptr;

    for (int i = 0; i < StreamList.size(); ++i) {

        if (StreamList.at(i)->ID == ID)
           pGetView = StreamList.at(i);
    }

    return pGetView;
}

int StreamContainer::GetVisibleStreamViewCount()
{
    int count = 0;
    for (int i = 0; i < StreamList.size(); ++i) {
        if (!StreamList.at(i)->getHidden())
           count++;
    }

    return count;
}

void  StreamContainer::UpdateTargetFPSToAllStream(int FPS)
{
    StreamView *pGetView = nullptr;

    for (int i = 0; i < StreamList.size(); ++i) {
        pGetView = StreamList.at(i);
        pGetView->changeTargetFPS(FPS);
    }
}
