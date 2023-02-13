#include "objectdetection.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "Object Detection"

//Select Network Model, Picke only one (TODO: will revise to be able to pick dynamically from App as selection list)
//#define USE_YOLOV5
#define USE_YOLOV7


#ifdef USE_YOLOV5
#define DETECTION_MODEL_INIT(ObjDetInfo, AppID)                 Yolov5mInitialize(ObjDetInfo, AppID)
#define DETECTION_MODEL_INFER_WORKER(ObjDetInfo, Data)          InferWorker(ObjDetInfo, Data)
#define DETECTION_MODEL_READ_OUTPUT_WORKER(ObjDetInfo, Data)    ReadOutputWorker(ObjDetInfo, Data)
#define DETECTION_MODEL_VISUALIZE_WORKER(ObjDetInfo, Data)      VisualizeWorker(ObjDetInfo, Data)
#endif

#ifdef USE_YOLOV7
#define DETECTION_MODEL_INIT(ObjDetInfo, AppID)                 Yolov7_Initialize(ObjDetInfo, AppID)
#define DETECTION_MODEL_INFER_WORKER(ObjDetInfo, Data)          Yolov7_InferWorker(ObjDetInfo, Data)
#define DETECTION_MODEL_READ_OUTPUT_WORKER(ObjDetInfo, Data)    Yolov7_ReadOutputWorker(ObjDetInfo, Data)
#define DETECTION_MODEL_VISUALIZE_WORKER(ObjDetInfo, Data)      Yolov7_VisualizeWorker(ObjDetInfo, Data)
#endif

ObjectDetection::ObjectDetection(QObject *parent)
    : AppBaseClass(parent)
{

    FrameCount = 0;
}

ObjectDetection::~ObjectDetection()
{
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
    DETECTION_MODEL_INIT(pObjDetInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        TimerFPSLimit.reset();

        if (!m_pImageInferQueue->empty()){

            timer.start();

            ResourceLock.lock();

            ObjectDetectionData* pData = new ObjectDetectionData();
            pData->VisualizedImage = m_pImageInferQueue->front().copy();
            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();

            //qDebug() << "1 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            DETECTION_MODEL_INFER_WORKER(pObjDetInfo, pData);

            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            DETECTION_MODEL_READ_OUTPUT_WORKER(pObjDetInfo, pData);

            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            DETECTION_MODEL_VISUALIZE_WORKER(pObjDetInfo, pData);

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

