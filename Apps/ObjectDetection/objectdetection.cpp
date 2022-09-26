#include "objectdetection.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#include "Apps/appsequencer.h"

ObjectDetection::ObjectDetection(QObject *parent)
    : AppBaseClass(parent)
{

}

const QString ObjectDetection::Name()
{
    return "Object Detection";
}

void ObjectDetection::ImageInfer(const QImage &frame)
{
    QMutexLocker locker(&ResourceLock);

    if (m_pImageInferQueue->size() > 10) {
        qDebug() << "ObjectDetection::ImageInfer Queue size = " << m_pImageInferQueue->size() << "exceeding limit, will drop frame";
        return;
    }

    m_pImageInferQueue->push_back(frame);

}


struct  Test {
    int                 func;
    bool                bParallelExecution;
    QImage              FinalImage;
};

struct  initTest {
    int                 test;
};

int MyInitFunction1(initTest* pInitData) {
    qDebug() << "MyInitFunction1";
    //QThread::currentThread()->sleep(5);
    return 0;
}

int MyTestFunction1(initTest* pInitData, Test* pShareData, const QImage &image) {
    qDebug() << "MyTestFunction1";
    //QThread::currentThread()->sleep(5);
    return 0;
}

int MyTestFunction2(initTest* pInitData, Test* pShareData, const QImage &image) {
    qDebug() << "MyTestFunction2";
    pShareData->FinalImage = image;
    QPainter qPainter(&pShareData->FinalImage);
    qPainter.setPen(QPen(Qt::red, 2));
    qPainter.drawRect(0,0,100,100);

    return 0;
}

int ShareDataCleanUp(Test* pShareData) {
    qDebug() << "ShareDataCleanUp";

    return 0;
}

void ObjectDetection::run()
{
    //QThread::currentThread()->setObjectName("Object Detection Thread");

    initTest* myinittest = new initTest;
    AppSequencer<Test,initTest> MyAppSequenceTest(myinittest);
    Test* MyData = new Test;
    MyAppSequenceTest.AddInitializer(MyInitFunction1);
    MyAppSequenceTest.AddSequencer(MyTestFunction1, false);
    MyAppSequenceTest.AddSequencer(MyTestFunction2, false);
    MyAppSequenceTest.setShareDataCleanupFunc(ShareDataCleanUp);

    while (!m_Terminate) {

        if (MyAppSequenceTest.SequencerStepRun() == eAppSeqErrorCode::SOME_DONE) {
            AppSequencer<Test, initTest>::TaskSequenceInfo  TaskInfo = MyAppSequenceTest.RetrieveSequenceTaskDoneInfo();
            emit sendAppResultImage(TaskInfo.pShareData->FinalImage, QList<QGraphicsItem*>());
            MyAppSequenceTest.RemoveSequenceTask(TaskInfo);
        }

        if (!m_pImageInferQueue->empty()) {

            ResourceLock.lock();

            QImage frame = m_pImageInferQueue->front();
            MyAppSequenceTest.SendSequenceTask(MyData, frame);

            //emit sendAppResultImage(frame, QList<QGraphicsItem*>());

            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();
        }
        else {
            QThread::currentThread()->usleep(500);
        }
    }

    delete m_pImageInferQueue;
    qDebug() << "ObjectDetection::run exiting";
}

typedef int (*myfunc)(const QImage &image);

void ObjectDetection::chas(myfunc func, const QImage &image)
{
    QList<myfunc> myFuncList({func});
    QtConcurrent::run(myFuncList.first(), image);

}
