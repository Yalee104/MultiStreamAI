#ifndef OBJECTDETECTION_H
#define OBJECTDETECTION_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/AppNetworkProcess/yolov5_process.h"
#include "Apps/AppNetworkProcess/yolov7_process.h"
#include "Apps/AppNetworkProcess/yolov5_faceDet_process.h"


class ObjectDetection : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             ObjectDetection(QObject *parent = nullptr);
    ~ObjectDetection();
    static const QString    AppName();
    const QString           GetAppName() override;
    const QString           GetSelectedNetwork() override;
    void                    SelectNetwork(QString NetworkName);

    bool AppContainSubMenu() override;
    QMenu* GetAppSubMenu() override;

    void ImageInfer(const QImage& frame) override;
    void run() Q_DECL_OVERRIDE;

private:
    int GetNetworkSelectionIndexMatchingName(QString NetworkName);

public slots:
    void NetworkSelectionChange(QAction *action);

protected:
    QMenu*                              pSubMenu = nullptr;
    NetworkInferenceDetectionObjInfo*   pObjDetInfo = nullptr;
    QMutex                              ResourceLock;
    bool                                bFrameInProcess = false;
    float                               FrameCount;
    int                                 SelectedNetworkMapping = 0;
    int                                 NewSelectedNetworkMapping = 0;
};


#endif // OBJECTDETECTION_H
