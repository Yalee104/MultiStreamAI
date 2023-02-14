#include "streamcontainer.h"
#include <QMenu>
#include <QDebug>
#include "cameraview.h"
#include "videoview.h"

StreamContainer::StreamContainer(QObject *parent)
    : QObject{parent}
{

}

StreamContainer::~StreamContainer()
{
    while (!StreamList.isEmpty())
        delete StreamList.takeFirst();
}

void StreamContainer::ConfigStream(StreamView* pStreamView, QJsonObject &ConfigInfo)
{
    if (dynamic_cast<const VideoView*>(pStreamView) != nullptr) {
        VideoView* pVideoView = dynamic_cast<VideoView*>(pStreamView);

        QJsonObject VideoJsonObj;

        QString VideoUrl = ConfigInfo["Url"].toString();
        QString SelectedAppName = ConfigInfo["SelectedAppName"].toString();

        pVideoView->loadSource(QUrl(VideoUrl));
        pVideoView->SelectApp(SelectedAppName);
    }

    if (dynamic_cast<const CameraView*>(pStreamView) != nullptr) {

        CameraView* pCameraView = dynamic_cast<CameraView*>(pStreamView);

        QString CameraDevice = ConfigInfo["CameraID"].toString();
        int     CameraPixelFormat = ConfigInfo["ViewSetting_PixelFormat"].toInt();
        int     CameraView_Width = ConfigInfo["ViewSetting_Width"].toInt();
        int     CameraView_Heights = ConfigInfo["ViewSetting_Height"].toInt();
        float   CameraView_MaxFPS = ConfigInfo["ViewSetting_MaxFPS"].toDouble();
        bool    CameraView_InvertImage = ConfigInfo["InvertImage"].toBool();

        QVideoFrame::PixelFormat ePixelFormat = static_cast<QVideoFrame::PixelFormat>(CameraPixelFormat);
        QCameraViewfinderSettings CameraViewSetting;
        CameraViewSetting.setMaximumFrameRate(CameraView_MaxFPS);
        CameraViewSetting.setPixelFormat(ePixelFormat);
        CameraViewSetting.setResolution(CameraView_Width, CameraView_Heights);

        pCameraView->StartCamera(CameraDevice, CameraView_InvertImage);
        pCameraView->ConfigCameraView(CameraViewSetting);

        QString SelectedAppName = ConfigInfo["SelectedAppName"].toString();
        pCameraView->SelectApp(SelectedAppName);
    }

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
                CameraJsonObj["Type"] = "Camera";

                CameraJsonObj["CameraID"] = pCameraView->m_CameraUniqueDeviceName;

                const QCameraViewfinderSettings CameraViewSetting = pCameraView->getCameraViewFinderSettings();
                CameraJsonObj["ViewSetting_PixelFormat"] = CameraViewSetting.pixelFormat();
                CameraJsonObj["ViewSetting_Width"] = CameraViewSetting.resolution().rwidth();
                CameraJsonObj["ViewSetting_Height"] = CameraViewSetting.resolution().rheight();
                CameraJsonObj["ViewSetting_MaxFPS"] = CameraViewSetting.maximumFrameRate();
                CameraJsonObj["InvertImage"] = pCameraView->m_InvertImage;

                CameraJsonObj["SelectedAppName"] = pCameraView->GetSelectedAppName();

                StreamDescriptor.append(CameraJsonObj);
            }
        }

        if (dynamic_cast<const VideoView*>(pGetView) != nullptr) {
            VideoView* pVideoView = dynamic_cast<VideoView*>(pGetView);
            QJsonObject VideoJsonObj;

            QString VideoUrl = pVideoView->m_SelectedMediaUrl.url();
            if (VideoUrl.size()) {
                VideoJsonObj["Type"] = "Video";
                VideoJsonObj["Url"] = VideoUrl;
                VideoJsonObj["SelectedAppName"] = pVideoView->GetSelectedAppName();

                StreamDescriptor.append(VideoJsonObj);
            }
        }

    }

    if (StreamDescriptor.count()) {
        StreamInfo["StreamDescriptor"] = StreamDescriptor;

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
