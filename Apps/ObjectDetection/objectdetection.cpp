#include "objectdetection.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "Object Detection"

/* Function pointer declaration */
typedef int (*pfnNetworkInit)(NetworkInferenceDetectionObjInfo* pInitData, std::string AppID);
typedef void (*pfnInfer)(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);
typedef void (*pfnReadOutput)(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);
typedef void (*pfnVisualize)(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);

typedef struct S_sNetworkMapping
{
    std::string NetworkName;
    pfnNetworkInit  NetworkInit;
    pfnInfer        InferProcess;
    pfnReadOutput   ReadOutputProcess;
    pfnVisualize    VisualizeProcess;

} sNetowrkMapping;

sNetowrkMapping ObjectDetectionSupportedNetworkMappingList[] = {

    {"yolov7tiny 500fps 80 Class", Yolov7_Initialize, Yolov7_InferWorker, Yolov7_ReadOutputWorker, Yolov7_VisualizeWorker},
    {"yolov5s 375fps Person/Face Class", Yolov5_PersonFace_Initialize, Yolov5_PersonFace_InferWorker, Yolov5_PersonFace_ReadOutputWorker, Yolov5_PersonFace_VisualizeWorker},
#ifdef QT_ON_x86
    {"yolov5m 80 Class", Yolov5mInitialize, InferWorker, ReadOutputWorker, VisualizeWorker},
#endif


    //Must be last
    {"NULL", NULL, NULL, NULL, NULL},
};



ObjectDetection::ObjectDetection(QObject *parent)
    : AppBaseClass(parent)
{
    pSubMenu = new QMenu(APP_NAME);
    SelectedNetworkMapping = 0; //o which is the first as default
    NewSelectedNetworkMapping = SelectedNetworkMapping; // Should be same as default
    FrameCount = 0;

    //Build Menu
    pSubMenu->clear();

    int totalSUpportedNetwork = sizeof(ObjectDetectionSupportedNetworkMappingList) / sizeof(ObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        QAction* NetworkSelection = pSubMenu->addAction(QString::fromStdString(ObjectDetectionSupportedNetworkMappingList[i].NetworkName));
        NetworkSelection->setCheckable(true);
        if (i == SelectedNetworkMapping)
            NetworkSelection->setChecked(true);
        NetworkSelection->setData(QString::fromStdString(ObjectDetectionSupportedNetworkMappingList[i].NetworkName));
    }

    connect(pSubMenu, SIGNAL(triggered(QAction*)), this, SLOT(NetworkSelectionChange(QAction*)));

}

ObjectDetection::~ObjectDetection()
{
    delete pSubMenu;
    //qDebug() << "~ObjectDetection()";
}

const QString ObjectDetection::AppName()
{
    return APP_NAME;
}

const QString ObjectDetection::GetAppName()
{
    return APP_NAME;
}

const QString ObjectDetection::GetSelectedNetwork()
{
    return QString::fromStdString(ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkName);
}

bool ObjectDetection::AppContainSubMenu()
{
    return true;
}

int ObjectDetection::GetNetworkSelectionIndexMatchingName(QString NetworkName)
{
    int index = -1;
    int totalSUpportedNetwork = sizeof(ObjectDetectionSupportedNetworkMappingList) / sizeof(ObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (NetworkName.compare(QString::fromStdString(ObjectDetectionSupportedNetworkMappingList[i].NetworkName)) == 0) {
            index = i;
            break;
        }
    }

    return index;
}

void ObjectDetection::SelectNetwork(QString NetworkName)
{
    int NetworkIndex = GetNetworkSelectionIndexMatchingName(NetworkName);
    if (NetworkIndex > 0) {
        NewSelectedNetworkMapping = NetworkIndex;
    }
}

QMenu* ObjectDetection::GetAppSubMenu()
{

    for (QAction* NetworkAction : pSubMenu->actions()) {
        int NetworkIndex = GetNetworkSelectionIndexMatchingName(NetworkAction->text());
        if (SelectedNetworkMapping != NetworkIndex) {
            NetworkAction->setChecked(false);
        }
        else {
            NetworkAction->setChecked(true);
        }
    }

    return pSubMenu;
}

void ObjectDetection::NetworkSelectionChange(QAction *action)
{
    qDebug() << action->data().toString();

    int totalSUpportedNetwork = sizeof(ObjectDetectionSupportedNetworkMappingList) / sizeof(ObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (action->data().toString().compare(QString::fromStdString(ObjectDetectionSupportedNetworkMappingList[i].NetworkName)) == 0) {
           NewSelectedNetworkMapping = i;
           break;
        }
    }
}


void ObjectDetection::ImageInfer(const QImage &frame)
{

    QMutexLocker locker(&ResourceLock);

    if (bFrameInProcess) {
        //static int mycount = 0;
        //qDebug() << "exceeding limit, will drop frame " << mycount++;
        return;
    }

    bFrameInProcess = true;
    m_pImageInferQueue->push_back(frame);

}


void ObjectDetection::run()
{
    QThread::currentThread()->setObjectName("Object Detection Thread");

    QElapsedTimer timer;
    Timer   TimerFPS;
    TimerMs TimerFPSLimit;

    pObjDetInfo = new NetworkInferenceDetectionObjInfo;
    pObjDetInfo->PerformaceFPS = 0; //Just an initial value
    ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        TimerFPSLimit.reset();

        if (NewSelectedNetworkMapping != SelectedNetworkMapping) {
            delete pObjDetInfo;
            SelectedNetworkMapping = NewSelectedNetworkMapping;
            pObjDetInfo = new NetworkInferenceDetectionObjInfo;
            pObjDetInfo->PerformaceFPS = 0; //Just an initial value
            ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());
            qDebug() << "Network CHANGED";
        }

        if (!m_pImageInferQueue->empty()){

            timer.start();

            ResourceLock.lock();

            AppImageData* pData = new AppImageData();
            pData->VisualizedImage = m_pImageInferQueue->front().copy();
            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();

            //qDebug() << "1 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].InferProcess(pObjDetInfo, pData);

            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].ReadOutputProcess(pObjDetInfo, pData);

            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].VisualizeProcess(pObjDetInfo, pData);

            //qDebug() << "4 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            QImage FinalImage = pData->VisualizedImage;
            emit sendAppResultImage(FinalImage, QList<QGraphicsItem*>());

            //qDebug() << "5 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;


            delete pData;

            //Send the signal we are done first before we sleep to limt the FPS.
            bFrameInProcess = false;

            //Limit FPS
            if (this->m_LimitFPS != 0) {
                double sleepfor = (1000.0f/this->m_LimitFPS) - TimerFPSLimit.getElapsedInMs();
                if (sleepfor > 0.0f)
                    QThread::currentThread()->msleep(sleepfor);
            }

            FrameCount++;

            if (TimerFPS.isTimePastSec(2.0)) {
                pObjDetInfo->PerformaceFPS = FrameCount/TimerFPS.getElapsedInSec();
                TimerFPS.reset();
                FrameCount = 0;
            }
        }
        else {
            QThread::currentThread()->usleep(100);
        }
    }

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    pHailoPipeline->ReleaseStreamChannel(0, pObjDetInfo->AppID);

    delete m_pImageInferQueue;
    delete pObjDetInfo;
    qDebug() << "ObjectDetection::run exiting";
}

