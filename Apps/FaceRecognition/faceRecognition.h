#ifndef FACE_RECONGNITION_H
#define FACE_RECONGNITION_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/AppNetworkProcess/yolov5_faceDet_process.h"
#include "Apps/AppNetworkProcess/arcface_process.h"

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

protected:

    QMenu*                              pSubMenu = nullptr;
    NetworkInferenceDetectionObjInfo*   pObjDetInfo = nullptr;
    NetworkInferenceBasedObjInfo*       pArcFaceInfo = nullptr;
    QMutex                              ResourceLock;
    bool                                bFrameInProcess = false;
    float                               FrameCount;

};


#endif // FACE_RECONGNITION_H
