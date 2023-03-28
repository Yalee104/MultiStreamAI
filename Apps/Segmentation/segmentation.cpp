#include "segmentation.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "Segmentation"

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

sNetowrkMapping SegmentationSupportedNetworkMappingList[] = {

    {"yolov5s instance seg 80 Class", Yolov5_Instance_Seg_Initialize, Yolov5_Instance_Seg_InferWorker, Yolov5_Instance_Seg_ReadOutputWorker, Yolov5_Instance_Seg_VisualizeWorker},

    //Must be last
    {"NULL", NULL, NULL, NULL, NULL},
};



Segmentation::Segmentation(QObject *parent)
    : AppBaseClass(parent)
{
    pSubMenu = new QMenu(APP_NAME);
    SelectedNetworkMapping = 0; //o which is the first as default
    NewSelectedNetworkMapping = SelectedNetworkMapping; // Should be same as default
    FrameCount = 0;

    //Build Menu
    pSubMenu->clear();

    int totalSUpportedNetwork = sizeof(SegmentationSupportedNetworkMappingList) / sizeof(SegmentationSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        QAction* NetworkSelection = pSubMenu->addAction(QString::fromStdString(SegmentationSupportedNetworkMappingList[i].NetworkName));
        NetworkSelection->setCheckable(true);
        if (i == SelectedNetworkMapping)
            NetworkSelection->setChecked(true);
        NetworkSelection->setData(QString::fromStdString(SegmentationSupportedNetworkMappingList[i].NetworkName));
    }

    connect(pSubMenu, SIGNAL(triggered(QAction*)), this, SLOT(NetworkSelectionChange(QAction*)));

}

Segmentation::~Segmentation()
{
    delete pSubMenu;
}

const QString Segmentation::AppName()
{
    return APP_NAME;
}

const QString Segmentation::GetAppName()
{
    return APP_NAME;
}

const QString Segmentation::GetSelectedNetwork()
{
    return QString::fromStdString(SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].NetworkName);
}

bool Segmentation::AppContainSubMenu()
{
    return true;
}

int Segmentation::GetNetworkSelectionIndexMatchingName(QString NetworkName)
{
    int index = -1;
    int totalSUpportedNetwork = sizeof(SegmentationSupportedNetworkMappingList) / sizeof(SegmentationSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (NetworkName.compare(QString::fromStdString(SegmentationSupportedNetworkMappingList[i].NetworkName)) == 0) {
            index = i;
            break;
        }
    }

    return index;
}

void Segmentation::SelectNetwork(QString NetworkName)
{
    int NetworkIndex = GetNetworkSelectionIndexMatchingName(NetworkName);
    if (NetworkIndex > 0) {
        NewSelectedNetworkMapping = NetworkIndex;
    }
}

QMenu* Segmentation::GetAppSubMenu()
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

void Segmentation::NetworkSelectionChange(QAction *action)
{
    qDebug() << action->data().toString();

    int totalSUpportedNetwork = sizeof(SegmentationSupportedNetworkMappingList) / sizeof(SegmentationSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (action->data().toString().compare(QString::fromStdString(SegmentationSupportedNetworkMappingList[i].NetworkName)) == 0) {
           NewSelectedNetworkMapping = i;
           break;
        }
    }
}


void Segmentation::ImageInfer(const QImage &frame)
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


void Segmentation::run()
{
    QThread::currentThread()->setObjectName("Segmentation Thread");

    QElapsedTimer timer;
    Timer   TimerFPS;
    TimerMs TimerFPSLimit;

    pObjDetInfo = new ObjectDetectionInfo;
    pObjDetInfo->PerformaceFPS = 0; //Just an initial value
    SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        TimerFPSLimit.reset();

        if (NewSelectedNetworkMapping != SelectedNetworkMapping) {
            delete pObjDetInfo;
            SelectedNetworkMapping = NewSelectedNetworkMapping;
            pObjDetInfo = new ObjectDetectionInfo;
            pObjDetInfo->PerformaceFPS = 0; //Just an initial value
            SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pObjDetInfo, this->m_AppID.toStdString());
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

            SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].InferProcess(pObjDetInfo, pData);

            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].ReadOutputProcess(pObjDetInfo, pData);

            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            SegmentationSupportedNetworkMappingList[SelectedNetworkMapping].VisualizeProcess(pObjDetInfo, pData);

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
    qDebug() << "Segmentation::run exiting";
}

