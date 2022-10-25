
#include <QDebug>
#include "appmanager.h"
#include "Apps/ObjectDetection/objectdetection.h"
#include "Apps/FaceRecognition/faceRecognition.h"

AppManager::AppManager(QObject *parent)
    : QObject{parent}
{
    m_pAppMenu = new QMenu("Apps");

    /*
     * ALL new apps must register here to AppMenu
    */

    //Register Apps: ObjectDetection
    QByteArray ClassName = m_AppsFactory.registerObject<ObjectDetection>();
    QAction *pObjectDetection = new QAction(m_pAppMenu);
    pObjectDetection->setText(ObjectDetection::Name());
    pObjectDetection->setData(ClassName);
    m_pAppMenu->addAction(pObjectDetection);

    //Register Apps: ObjectDetection
    ClassName = m_AppsFactory.registerObject<FaceRecognition>();
    QAction *pFaceRecognition = new QAction(m_pAppMenu);
    pFaceRecognition->setText(FaceRecognition::Name());
    pFaceRecognition->setData(ClassName);
    m_pAppMenu->addAction(pFaceRecognition);

    //TODO: To add more Apps simply copy any of above register apps and make
    //      necessary changes to reflect the added app class


    //Connect selection execution
    connect(m_pAppMenu, SIGNAL(triggered(QAction*)), this, SLOT(AppSelectionTrigger(QAction*)));
}

AppManager::~AppManager()
{
    ReleaseCurrentApp();
}

QMenu *AppManager::getAppMenu() const
{
    return m_pAppMenu;
}

bool AppManager::AppSelected()
{
    if (m_pAppRunnableObject != nullptr)
        return true;

    return false;
}

void AppManager::AppImageInfer(const QImage &frame)
{
    QMutexLocker locker(&m_AppAccessMutex);

    if (m_pAppRunnableObject)
        m_pAppRunnableObject->ImageInfer(frame);
}

void AppManager::ReleaseCurrentApp()
{
    if (m_pAppRunnableObject) {
        m_pAppRunnableObject->setTerminate(true);
    }
}

void AppManager::AppSelectionTrigger(QAction *action)
{
    QMutexLocker locker(&m_AppAccessMutex);

    ReleaseCurrentApp();

    // If the threadpool maximum allowed thread is low we need to increase the number otherwise it will creash the app
    // TODO: Alternatively we can also limit this to prevent application adding too much of stream
    int CurrentActiveThreadCount = QThreadPool::globalInstance()->activeThreadCount();
    int MaximumAvailableThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    //qDebug() << CurrentActiveThreadCount;
    //qDebug() << MaximumAvailableThreadCount;
    if (CurrentActiveThreadCount+1 >=  MaximumAvailableThreadCount)
        QThreadPool::globalInstance()->setMaxThreadCount(MaximumAvailableThreadCount+2);

    //Selected App and Start it in thread pool
    m_pAppRunnableObject = dynamic_cast<AppBaseClass*>(m_AppsFactory.createObject(action->data().toByteArray()));
    m_pAppRunnableObject->m_AppID = this->m_AppID;
    QThreadPool::globalInstance()->start(m_pAppRunnableObject);
    //Redirect the signal of App result image to AppManager sendImage signal

    connect(m_pAppRunnableObject, SIGNAL(sendAppResultImage(QImage, QList<QGraphicsItem*>)), this, SIGNAL(sendImage(QImage, QList<QGraphicsItem*>)));

}
