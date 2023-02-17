#include "objectdetection.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "Object Detection"

/* Function pointer declaration */
typedef int (*pfnNetworkInit)(ObjectDetectionInfo* pInitData, std::string AppID);
typedef void (*pfnInfer)(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);
typedef void (*pfnReadOutput)(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);
typedef void (*pfnVisualize)(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

typedef struct S_sNetworkMapping
{
    std::string NetworkName;
    pfnNetworkInit  NetworkInit;
    pfnInfer        InferProcess;
    pfnReadOutput   ReadOutputProcess;
    pfnVisualize    VisualizeProcess;

} sNetowrkMapping;

sNetowrkMapping SupportedNetworkMappingList[] = {

    {"yolov7s 500fps 80 Class", Yolov7_Initialize, Yolov7_InferWorker, Yolov7_ReadOutputWorker, Yolov7_VisualizeWorker},
    {"yolov5m 80 Class", Yolov5mInitialize, InferWorker, ReadOutputWorker, VisualizeWorker},

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

    int totalSUpportedNetwork = sizeof(SupportedNetworkMappingList) / sizeof(SupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        QAction* NetworkSelection = pSubMenu->addAction(QString::fromStdString(SupportedNetworkMappingList[i].NetworkName));
        NetworkSelection->setCheckable(true);
        if (i == SelectedNetworkMapping)
            NetworkSelection->setChecked(true);
        NetworkSelection->setData(QString::fromStdString(SupportedNetworkMappingList[i].NetworkName));
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
    return QString::fromStdString(SupportedNetworkMappingList[SelectedNetworkMapping].NetworkName);
}

bool ObjectDetection::AppContainSubMenu()
{
    return true;
}

int ObjectDetection::GetNetworkSelectionIndexMatchingName(QString NetworkName)
{
    int index = -1;
    int totalSUpportedNetwork = sizeof(SupportedNetworkMappingList) / sizeof(SupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (NetworkName.compare(QString::fromStdString(SupportedNetworkMappingList[i].NetworkName)) == 0) {
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

    int totalSUpportedNetwork = sizeof(SupportedNetworkMappingList) / sizeof(SupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (action->data().toString().compare(QString::fromStdString(SupportedNetworkMappingList[i].NetworkName)) == 0) {
           NewSelectedNetworkMapping = i;
           break;
        }
    }
}


void ObjectDetection::ImageInfer(const QImage &frame)
{
    static int mycount = 0;
    QMutexLocker locker(&ResourceLock);

    if (bFrameInProcess) {
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

    pObjDetInfo = new ObjectDetectionInfo;
    pObjDetInfo->PerformaceFPS = 0; //Just an initial value
    SupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        TimerFPSLimit.reset();

        if (NewSelectedNetworkMapping != SelectedNetworkMapping) {
            delete pObjDetInfo;
            SelectedNetworkMapping = NewSelectedNetworkMapping;
            pObjDetInfo = new ObjectDetectionInfo;
            pObjDetInfo->PerformaceFPS = 0; //Just an initial value
            SupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());
            qDebug() << "Network CHANGED";
        }

        if (!m_pImageInferQueue->empty()){

            timer.start();

            ResourceLock.lock();

            ObjectDetectionData* pData = new ObjectDetectionData();
            pData->VisualizedImage = m_pImageInferQueue->front().copy();
            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();

            //qDebug() << "1 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            SupportedNetworkMappingList[SelectedNetworkMapping].InferProcess(pObjDetInfo, pData);

            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            SupportedNetworkMappingList[SelectedNetworkMapping].ReadOutputProcess(pObjDetInfo, pData);

            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            SupportedNetworkMappingList[SelectedNetworkMapping].VisualizeProcess(pObjDetInfo, pData);

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

