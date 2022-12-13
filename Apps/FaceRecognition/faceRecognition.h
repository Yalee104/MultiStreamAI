#ifndef FACE_RECONGNITION_H
#define FACE_RECONGNITION_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/appsequencer.h"
#include "Apps/FaceRecognition/yolov5_faceDet_process.h"
#include "Apps/FaceRecognition/arcface_process.h"

class FaceRecognition : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             FaceRecognition(QObject *parent = nullptr);
    ~FaceRecognition();

    static const QString    AppName();
    const QString           GetAppName() override;

    bool AppContainSubMenu() override;
    QMenu* GetAppSubMenu() override;

    void ImageInfer(const QImage& frame) override;

    void run() Q_DECL_OVERRIDE;

public slots:

    void ReloadFaceDatabase();

signals:

    void SendToInfer(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

protected:

    QMenu*                          pSubMenu = nullptr;
    ObjectDetectionInfo*            pObjDetInfo = nullptr;
    FaceRecognitionInfo*            pArcFaceInfo = nullptr;
    QMutex                          ResourceLock;
    bool                            bFrameInProcess = false;
    float                           FrameCount;

};


#endif // FACE_RECONGNITION_H
