#include "objectdetection.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>


ObjectDetection::ObjectDetection(QObject *parent)
    : AppBaseClass(parent)
{

    FrameCount = 0;
}

const QString ObjectDetection::Name()
{
    return "Object Detection";
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

    //TODO: Need a way to delete pObjDetInfo
    pObjDetInfo = new ObjectDetectionInfo;
    pObjDetInfo->PerformaceFPS = 0; //Just an initial value
    Yolov5mInitialize(pObjDetInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        if (!m_pImageInferQueue->empty()){

            timer.start();

            ResourceLock.lock();

            ObjectDetectionData* pData = new ObjectDetectionData();
            pData->VisualizedImage = m_pImageInferQueue->front().copy();
            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();

            //qDebug() << "1 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            InferWorker(pObjDetInfo, pData);

            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ReadOutputWorker(pObjDetInfo, pData);

            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            VisualizeWorker(pObjDetInfo, pData);

            //qDebug() << "4 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            QImage FinalImage = pData->VisualizedImage;
            emit sendAppResultImage(FinalImage, QList<QGraphicsItem*>());

            //qDebug() << "5 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;


            delete pData;
            bFrameInProcess = false;
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

    delete m_pImageInferQueue;
    qDebug() << "ObjectDetection::run exiting";
}

