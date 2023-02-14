
#include <QDebug>
#include "appmanager.h"
#include "Apps/ObjectDetection/objectdetection.h"
#include "Apps/FaceRecognition/faceRecognition.h"

AppManager::AppManager(QObject *parent)
    : QObject{parent}
{
    m_pAppMenu = new QMenu("Apps");

    //Connect selection execution
    connect(m_pAppMenu, SIGNAL(triggered(QAction*)), this, SLOT(AppSelectionTrigger(QAction*)));
}

AppManager::~AppManager()
{
    QMutexLocker locker(&m_AppAccessMutex);

    ReleaseCurrentApp();
    ReleaseAppMenu();

}

QString AppManager::GetSelectedAppName()
{
    QString AppName = "";
    if (m_pAppRunnableObject != nullptr) {
        AppName = m_pAppRunnableObject->GetAppName();
    }

    return AppName;
}


void  AppManager::StartApp(QByteArray AppClass)
{
    // If the threadpool maximum allowed thread is low we need to increase the number otherwise it will creash the app
    // TODO: Alternatively we can also limit this to prevent application adding too much of stream
    int CurrentActiveThreadCount = QThreadPool::globalInstance()->activeThreadCount();
    int MaximumAvailableThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    //qDebug() << CurrentActiveThreadCount;
    //qDebug() << MaximumAvailableThreadCount;
    if (CurrentActiveThreadCount+1 >=  MaximumAvailableThreadCount)
        QThreadPool::globalInstance()->setMaxThreadCount(MaximumAvailableThreadCount+2);

    //Selected App and Start it in thread pool
    m_pAppRunnableObject = dynamic_cast<AppBaseClass*>(m_AppsFactory.createObject(AppClass));
    m_pAppRunnableObject->m_AppID = this->m_AppID;
    QThreadPool::globalInstance()->start(m_pAppRunnableObject);
    //Redirect the signal of App result image to AppManager sendImage signal

    connect(m_pAppRunnableObject, SIGNAL(sendAppResultImage(QImage, QList<QGraphicsItem*>)), this, SIGNAL(sendImage(QImage, QList<QGraphicsItem*>)));

}



void AppManager::LaunchApp(QString AppName)
{
    if (AppName.compare(ObjectDetection::AppName()) == 0) {
        QByteArray ClassName = m_AppsFactory.registerObject<ObjectDetection>();
        StartApp(ClassName);
    }

    if (AppName.compare(FaceRecognition::AppName()) == 0) {
        QByteArray ClassName = m_AppsFactory.registerObject<FaceRecognition>();
        StartApp(ClassName);
    }

}


void AppManager::ReGenerateMenu()
{
    m_pAppMenu->clear();

    /*
     * ALL new apps must register here to AppMenu
    */

    //Register Apps: ObjectDetection
    QByteArray ClassName = m_AppsFactory.registerObject<ObjectDetection>();
    QAction *pObjectDetection = new QAction(m_pAppMenu);
    pObjectDetection->setText(ObjectDetection::AppName());
    pObjectDetection->setData(ClassName);
    m_pAppMenu->addAction(pObjectDetection);

    //Register Apps: Face Recognition
    ClassName = m_AppsFactory.registerObject<FaceRecognition>();
    QAction *pFaceRecognition = new QAction(m_pAppMenu);
    pFaceRecognition->setText(FaceRecognition::AppName());
    pFaceRecognition->setData(ClassName);
    m_pAppMenu->addAction(pFaceRecognition);

    //To add more Apps simply copy any of above register apps and make
    //necessary changes to reflect the added app class
    //TODO: ADD HERE



    //Auto replace the app submenu (if any) for current app (if already created)
    QList<QAction*> ActionList = m_pAppMenu->actions();
    for (QAction* menuAction : ActionList) {
        if (m_pAppRunnableObject) {
            if (menuAction->text().compare(m_pAppRunnableObject->GetAppName()) == 0) {
                if (m_pAppRunnableObject->AppContainSubMenu()) {

                    m_pAppMenu->removeAction(menuAction);
                    m_pAppMenu->addMenu(m_pAppRunnableObject->GetAppSubMenu());
                    break;
                }
            }
        }
    }
}

void AppManager::ReleaseCurrentApp()
{
    if (m_pAppRunnableObject) {
        m_pAppRunnableObject->setTerminate(true);
        //NOTE: m_pAppRunnableObject will self destroy after runnable.
    }
}

void AppManager::ReleaseAppMenu()
{
    if (m_pAppMenu) {
        delete m_pAppMenu;
        m_pAppMenu = nullptr;
    }
}


QMenu *AppManager::getAppMenu()
{
    ReGenerateMenu();
    return m_pAppMenu;
}

bool AppManager::AppSelected()
{
    if (m_pAppRunnableObject != nullptr)
        return true;

    return false;
}

void  AppManager::AppLimitFPS(int FPS)
{
    if (m_pAppRunnableObject != nullptr){
        m_pAppRunnableObject->m_LimitFPS = FPS;
    }

}

void AppManager::AppImageInfer(const QImage &frame)
{
    QMutexLocker locker(&m_AppAccessMutex);

    if (m_pAppRunnableObject)
        m_pAppRunnableObject->ImageInfer(frame);
}

void AppManager::AppSelectionTrigger(QAction *action)
{
    QMutexLocker locker(&m_AppAccessMutex);

    //If data is null we ignore as this is provably app submenu
    if (action->data().isNull())
        return;

    //If data exist but its not registered then we won't be able to create the app, ignore it as its most likely app submenu
    if (!m_AppsFactory.isObjectRegistered(action->data().toByteArray()))
        return;

    //If selected app is already current created app, we skip.
    if (m_pAppRunnableObject) {
        if (action->text().compare(m_pAppRunnableObject->GetAppName()) == 0) {
            return;
        }
    }

    ReleaseCurrentApp();

    StartApp(action->data().toByteArray());

}
