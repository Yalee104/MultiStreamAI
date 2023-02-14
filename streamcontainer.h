#ifndef STREAMCONTAINER_H
#define STREAMCONTAINER_H

#include <QObject>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "streamview.h"

enum class eStreamViewUpdate { HIDDEN, REMOVE };
enum class eStreamViewType { CAMERA, VIDEO };


class StreamContainer : public QObject
{
    Q_OBJECT
public:
    explicit StreamContainer(QObject *parent = nullptr);
    ~StreamContainer();
    StreamView*     CreateNewStream(QString NewID, eStreamViewType StreamType);
    void            ConfigStream(StreamView* pStreamView, QJsonObject &ConfigInfo);
    void            DeleteStream(QString ID);
    QList<QString>  GetAllStreamViewID();
    StreamView*     GetStreamViewByID(QString ID);
    int             GetVisibleStreamViewCount();
    void            UpdateTargetFPSToAllStream(int FPS);
    void            SaveStreamInfoToFile(QString FileName);

signals:
    Q_INVOKABLE void ContainerViewUpdateRequest(QString ID, eStreamViewUpdate Request);


public:
    QList<StreamView*> StreamList;


};

#endif // STREAMCONTAINER_H
