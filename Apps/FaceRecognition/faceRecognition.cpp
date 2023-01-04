#include "faceRecognition.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "Face Recognition"

#define FACE_IMAGE_DB_PATH "FaceImageDb"

/*
 * NOTE: This app might crash if build under Windows VS in DEBUGT Mode due to possible issue related to
 *       https://developercommunity.visualstudio.com/t/calling-stdsort-with-a-vector-using-c-like-array-s/546861
 *
 *       RELEASE mode build and execution works well, so highly suggest to build in release mode when tracker is used.
 *
 */


FaceRecognition::FaceRecognition(QObject *parent)
    : AppBaseClass(parent)
{
    pSubMenu = new QMenu(APP_NAME);

    FrameCount = 0;
}

FaceRecognition::~FaceRecognition()
{
    delete pSubMenu;
}


const QString FaceRecognition::AppName()
{
    return APP_NAME;
}

const QString FaceRecognition::GetAppName()
{
    return APP_NAME;
}


bool FaceRecognition::AppContainSubMenu()
{
    return true;
}


QMenu* FaceRecognition::GetAppSubMenu()
{
    pSubMenu->clear();

    pSubMenu->addAction("Reload Database", this, SLOT(ReloadFaceDatabase()));

    return pSubMenu;
}

void FaceRecognition::ReloadFaceDatabase()
{
    //TODO: Here we reload the face database

    qDebug() << "ReloadFaceDatabase";
}

void FaceRecognition::ImageInfer(const QImage &frame)
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


void FaceRecognition::run()
{
    QThread::currentThread()->setObjectName("Object Detection Thread");

    QElapsedTimer timer;
    Timer   TimerFPS;
    TimerMs TimerFPSLimit;

    pObjDetInfo = new ObjectDetectionInfo;
    pObjDetInfo->PerformaceFPS = 0; //Just an initial value
    Yolov5_PersonFace_Initialize(pObjDetInfo, this->m_AppID.toStdString());

    pArcFaceInfo = new FaceRecognitionInfo;
    Arcface_Initialize(pArcFaceInfo, this->m_AppID.toStdString());

    Arface_BuildFaceDB(QString(FACE_IMAGE_DB_PATH), pArcFaceInfo);

    //Arcface_Test("FaceImageTest", "Aaron.jpg", pArcFaceInfo);
    //Arcface_Test("FaceImageTest", "letter_boxed_aaron.jpg", pArcFaceInfo);


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

            Yolov5_PersonFace_InferWorker(pObjDetInfo, pData);


            //qDebug() << "2 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            Yolov5_PersonFace_ReadOutputWorker(pObjDetInfo, pData);


            //qDebug() << "3 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ArcFace_InferWorker(pArcFaceInfo, pObjDetInfo, pData);


            //qDebug() << "4 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ArcFace_ReadOutputWorker(pArcFaceInfo, pData);

            //qDebug() << "5 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            ArcFace_VisualizeWorker(pObjDetInfo, pData);


            //qDebug() << "6 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            QImage FinalImage = pData->VisualizedImage;
            emit sendAppResultImage(FinalImage, QList<QGraphicsItem*>());


            //qDebug() << "6 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            delete pData;
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
    pHailoPipeline->ReleaseStreamChannel(0, pArcFaceInfo->AppID);

    delete m_pImageInferQueue;
    delete pObjDetInfo;
    delete pArcFaceInfo;



    qDebug() << "FaceRecognition::run exiting";
}

