#ifndef STREAMCONTAINER_H
#define STREAMCONTAINER_H

#include <QObject>
#include "streamview.h"

enum class eStreamViewUpdate { HIDDEN, REMOVE };
enum class eStreamViewType { CAMERA, VIDEO };


class StreamContainer : public QObject
{
    Q_OBJECT
public:
    explicit StreamContainer(QObject *parent = nullptr);
    ~StreamContainer();
    StreamView*   CreateNewStream(QString NewID, eStreamViewType StreamType);
    void          DeleteStream(QString ID);
    StreamView*   GetStreamViewByID(QString ID);
    int           GetVisibleStreamViewCount();

signals:
    Q_INVOKABLE void ContainerViewUpdateRequest(QString ID, eStreamViewUpdate Request);


public:
    QList<StreamView*> StreamList;


};

#endif // STREAMCONTAINER_H
