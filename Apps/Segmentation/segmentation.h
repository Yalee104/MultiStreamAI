#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/AppNetworkProcess/yolov5_instance_seg_process.h"



class Segmentation : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             Segmentation(QObject *parent = nullptr);
    ~Segmentation();
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

signals:

    void SendToInfer(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

protected:
    QMenu*                          pSubMenu = nullptr;
    ObjectDetectionInfo*            pObjDetInfo = nullptr;
    QMutex                          ResourceLock;
    bool                            bFrameInProcess = false;
    float                           FrameCount;
    int                             SelectedNetworkMapping = 0;
    int                             NewSelectedNetworkMapping = 0;
};


#endif // SEGMENTATION_H
