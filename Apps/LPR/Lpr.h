#ifndef LPR_H
#define LPR_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "Apps/appbaseclass.h"
#include "Apps/AppNetworkProcess/yolov5_process.h"
#include "Apps/AppNetworkProcess/yolov7tiny_process.h"
#include "Apps/AppNetworkProcess/yolov5_vehicle_process.h"
#include "Apps/AppNetworkProcess/yolov4Tiny_license_plate_det_process.h"
#include "Apps/AppNetworkProcess/lprnet_process.h"


class LPR : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             LPR(QObject *parent = nullptr);
    ~LPR();
    static const QString    AppName();
    const QString           GetAppName() override;
    const QString           GetSelectedNetwork() override;
    void                    SelectNetwork(QString NetworkName);

    bool AppContainSubMenu() override;
    QMenu* GetAppSubMenu() override;

    void ImageInfer(const QImage& frame) override;
    void run() Q_DECL_OVERRIDE;

private:

    template <typename T>
    QImage PadImage(const QImage & source, int targetWidth, int TargetHeight, T padValue);

    int GetNetworkSelectionIndexMatchingName(QString NetworkName);

    void ProcessTrackerAndExtractDetectedVehiclesForLPD(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo,
                                                        NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                                        AppImageData* pImageData, 
                                                        std::vector<int>    &SupportedClassList);

    void ProcessTrackerAndPrepareDetectedVehicleWithLpdForLPR(  NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo,
                                                                NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                                                AppImageData* pImageData, 
                                                                std::vector<int>    &SupportedClassList);

    void ProcessTrackerAndExtractDetectedLpdForLPR( NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, 
                                                    NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, 
                                                    AppImageData* pImageData);                                                                

    void ProcessTrackerOfLprResults(NetworkInferenceBasedObjInfo*     pLicensePlateRecognitionInfo,  
                                    NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                    AppImageData*                     pImageData);

    void VisualizeResult(NetworkInferenceDetectionObjInfo* pVehicleDetInfo, AppImageData* pImageData);                                                      

public slots:
    void NetworkSelectionChange(QAction *action);


protected:
    QMenu*                              pSubMenu = nullptr;
    NetworkInferenceDetectionObjInfo*   pVehicleObjDetInfo = nullptr;
    NetworkInferenceDetectionObjInfo*   pLicensePlateObjDetInfo = nullptr;
    NetworkInferenceBasedObjInfo*       pLicensePlateRecognitionInfo = nullptr;
    QMutex                              ResourceLock;
    bool                                bFrameInProcess = false;
    float                               FrameCount;
    int                                 SelectedNetworkMapping = 0;
    int                                 NewSelectedNetworkMapping = 0;
    std::vector<int>                    licese_plate_det_tracked_ids_temp;

};


#endif // LPR_H
