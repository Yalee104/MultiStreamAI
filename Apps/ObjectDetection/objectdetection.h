#ifndef OBJECTDETECTION_H
#define OBJECTDETECTION_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/ObjectDetection/yolov5_process.h"

class ObjectDetection : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             ObjectDetection(QObject *parent = nullptr);
    ~ObjectDetection();
    static const QString    AppName();
    const QString           GetAppName() override;


    void ImageInfer(const QImage& frame) override;
    void run() Q_DECL_OVERRIDE;

signals:

    void SendToInfer(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

protected:

    ObjectDetectionInfo*            pObjDetInfo = nullptr;
    QMutex                          ResourceLock;
    bool                            bFrameInProcess = false;
    float                           FrameCount;

};


#endif // OBJECTDETECTION_H
