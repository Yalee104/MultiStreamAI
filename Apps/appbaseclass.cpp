#include "appbaseclass.h"
#include <QDebug>

AppBaseClass::AppBaseClass(QObject *parent)
    : QObject{parent}
{
    m_pImageInferQueue = new QList<QImage>();

    qRegisterMetaType<QList<QGraphicsItem*>>("QList<QGraphicsItem*>");

}

AppBaseClass::~AppBaseClass()
{
    //TODO: We delete this from its child class but it should be here
    //      However, doing this here will generate bunch of compilation error, try to find out why
    //delete m_pImageInferQueue;
}

bool AppBaseClass::AppContainSubMenu()
{
    return false;
}

QMenu*  AppBaseClass::GetAppSubMenu()
{
    return nullptr;
}

void AppBaseClass::setTerminate(bool newTerminate)
{
    m_Terminate = newTerminate;
}
