#include "streamcontainer.h"
#include <QMenu>
#include <QDebug>
#include "cameraview.h"
#include "videoview.h"

StreamContainer::StreamContainer(QObject *parent)
    : QObject{parent}
{

}

StreamContainer::~StreamContainer()
{
    while (!StreamList.isEmpty())
        delete StreamList.takeFirst();
}


StreamView* StreamContainer::CreateNewStream(QString NewID, eStreamViewType StreamType)
{
    StreamView* pNewStreamView = nullptr;
    if (StreamType == eStreamViewType::CAMERA) {
        pNewStreamView = new CameraView(NewID);
    }
    else {
        pNewStreamView = new VideoView(NewID);
    }

    StreamList.push_back(pNewStreamView);

    return pNewStreamView;
}

void StreamContainer::DeleteStream(QString ID)
{
    for (int i = 0; i < StreamList.size(); ++i) {

        if (StreamList.at(i)->ID == ID) {
           StreamView *pGetView = StreamList.at(i);
           StreamList.removeAt(i);
           pGetView->disconnect();
           pGetView->deleteLater();

           break;
        }
    }
}


StreamView *StreamContainer::GetStreamViewByID(QString ID)
{
    StreamView *pGetView = nullptr;

    for (int i = 0; i < StreamList.size(); ++i) {

        if (StreamList.at(i)->ID == ID)
           pGetView = StreamList.at(i);
    }

    return pGetView;
}

int StreamContainer::GetVisibleStreamViewCount()
{
    int count = 0;
    for (int i = 0; i < StreamList.size(); ++i) {
        if (!StreamList.at(i)->getHidden())
           count++;
    }

    return count;
}


