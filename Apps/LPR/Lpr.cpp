#include "Lpr.h"
#include <QImage>
#include <QPainter>
#include <QtConcurrent>
#include <QDebug>

#define APP_NAME    "LPR"


#define LICENSE_PLATE_DET_TRACKER_INITIAL        (0)
#define LICENSE_PLATE_DET_TRACKER_NEED_PREDICT   (1)
#define LICENSE_PLATE_DET_TRACKER_IDENTIFIED     (2)
#define LICENSE_PLATE_DET_TRACKER_NO_PLATE_FOUND (3)
#define LICENSE_PLATE_RECOGNITION_IDENTIFIED     (4)

#define VEHICLE_LICENSE_PLATE_NOT_FOUNT_COUNT_MAX  (45.0f)

#define SAVE_LICENCE_PLATE_IMAGE        0   // 1: Keep license plate image to file, 0: not save

/* Function pointer declaration */
typedef int (*pfnNetworkInit)(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::string AppID);
typedef void (*pfnInfer)(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);
typedef void (*pfnReadOutput)(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);
typedef void (*pfnVisualize)(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);

enum {
    SUPPORTED_CLASS_DUMMY = 0,
    YOLOV5M_VEHICLE_HEF_SUPPORTED_CLASS_VEHICLE = 1,

    YOLO_COCO_80_HEF_SUPPRTED_CLASS_CAR = 3,
    YOLO_COCO_80_HEF_SUPPRTED_CLASS_MOTOCYCLE = 4,
    YOLO_COCO_80_HEF_SUPPRTED_CLASS_TRUCK = 8,

};

typedef struct S_LprNetworkMapping
{
    std::string         NetworkName;
    pfnNetworkInit      NetworkInit;
    pfnInfer            InferProcess;
    pfnReadOutput       ReadOutputProcess;
    pfnVisualize        VisualizeProcess;
    std::vector<int>    SupportedClassList;

} sLprNetowrkMapping;


sLprNetowrkMapping LprObjectDetectionSupportedNetworkMappingList[] = {


    {"yolov7tiny 500fps 80 Class",  Yolov7Tiny_Initialize, Yolov7Tiny_InferWorker, Yolov7Tiny_ReadOutputWorker, Yolov7Tiny_VisualizeWorker,
     std::vector<int> {YOLO_COCO_80_HEF_SUPPRTED_CLASS_CAR, YOLO_COCO_80_HEF_SUPPRTED_CLASS_TRUCK}},

    //Bad Performance, not recommended
    //{"yolov5m 80FPS Vehicle 1 Class",   Yolov5m_Vehicle_Initialize, Yolov5m_Vehicle_InferWorker, Yolov5m_Vehicle_ReadOutputWorker, Yolov5m_Vehicle_VisualizeWorker,
    //                                    std::vector<int> {YOLOV5M_VEHICLE_HEF_SUPPORTED_CLASS_VEHICLE}},

    //Must be last
    {"NULL", NULL, NULL, NULL, NULL, std::vector<int>{0}},
};



LPR::LPR(QObject *parent)
    : AppBaseClass(parent)
{
    pSubMenu = new QMenu(APP_NAME);
    SelectedNetworkMapping = 0; //o which is the first as default
    NewSelectedNetworkMapping = SelectedNetworkMapping; // Should be same as default
    FrameCount = 0;

    //Build Menu
    pSubMenu->clear();

    int totalSUpportedNetwork = sizeof(LprObjectDetectionSupportedNetworkMappingList) / sizeof(LprObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        QAction* NetworkSelection = pSubMenu->addAction(QString::fromStdString(LprObjectDetectionSupportedNetworkMappingList[i].NetworkName));
        NetworkSelection->setCheckable(true);
        if (i == SelectedNetworkMapping)
            NetworkSelection->setChecked(true);
        NetworkSelection->setData(QString::fromStdString(LprObjectDetectionSupportedNetworkMappingList[i].NetworkName));
    }

    connect(pSubMenu, SIGNAL(triggered(QAction*)), this, SLOT(NetworkSelectionChange(QAction*)));

}

LPR::~LPR()
{
    delete pSubMenu;
    //qDebug() << "~LPR()";
}

const QString LPR::AppName()
{
    return APP_NAME;
}

const QString LPR::GetAppName()
{
    return APP_NAME;
}

const QString LPR::GetSelectedNetwork()
{
    return QString::fromStdString(LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkName);
}

bool LPR::AppContainSubMenu()
{
    return true;
}

int LPR::GetNetworkSelectionIndexMatchingName(QString NetworkName)
{
    int index = -1;
    int totalSUpportedNetwork = sizeof(LprObjectDetectionSupportedNetworkMappingList) / sizeof(LprObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (NetworkName.compare(QString::fromStdString(LprObjectDetectionSupportedNetworkMappingList[i].NetworkName)) == 0) {
            index = i;
            break;
        }
    }

    return index;
}

void LPR::SelectNetwork(QString NetworkName)
{
    int NetworkIndex = GetNetworkSelectionIndexMatchingName(NetworkName);
    if (NetworkIndex > 0) {
        NewSelectedNetworkMapping = NetworkIndex;
    }
}

QMenu* LPR::GetAppSubMenu()
{

    for (QAction* NetworkAction : pSubMenu->actions()) {
        int NetworkIndex = GetNetworkSelectionIndexMatchingName(NetworkAction->text());
        if (SelectedNetworkMapping != NetworkIndex) {
            NetworkAction->setChecked(false);
        }
        else {
            NetworkAction->setChecked(true);
        }
    }

    return pSubMenu;
}

void LPR::NetworkSelectionChange(QAction *action)
{
    qDebug() << action->data().toString();

    int totalSUpportedNetwork = sizeof(LprObjectDetectionSupportedNetworkMappingList) / sizeof(LprObjectDetectionSupportedNetworkMappingList[0]);
    for (int i = 0; i < (totalSUpportedNetwork - 1); i++) {
        if (action->data().toString().compare(QString::fromStdString(LprObjectDetectionSupportedNetworkMappingList[i].NetworkName)) == 0) {
           NewSelectedNetworkMapping = i;
           break;
        }
    }
}


void LPR::ImageInfer(const QImage &frame)
{

    QMutexLocker locker(&ResourceLock);

    if (bFrameInProcess) {
        //static int mycount = 0;
        //qDebug() << "exceeding limit, will drop frame " << mycount++;
        return;
    }

    bFrameInProcess = true;
    m_pImageInferQueue->push_back(frame);

}


void LPR::run()
{
    QThread::currentThread()->setObjectName("Object Detection Thread");

    QElapsedTimer timer;
    Timer   TimerFPS;
    TimerMs TimerFPSLimit;

    pVehicleObjDetInfo = new NetworkInferenceDetectionObjInfo;
    pVehicleObjDetInfo->PerformaceFPS = 0; //Just an initial value
    LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pVehicleObjDetInfo, this->m_AppID.toStdString());

    pLicensePlateObjDetInfo = new NetworkInferenceDetectionObjInfo;
    pLicensePlateObjDetInfo->PerformaceFPS = 0; //Just an initial value
    Yolov4TinyLicensePlate_Initialize(pLicensePlateObjDetInfo, this->m_AppID.toStdString());

    pLicensePlateRecognitionInfo = new NetworkInferenceBasedObjInfo;
    pLicensePlateRecognitionInfo->PerformaceFPS = 0; //Just an initial value
    LprNet_Initialize(pLicensePlateRecognitionInfo, this->m_AppID.toStdString());

    TimerFPS.reset();
    while (!m_Terminate) {

        TimerFPSLimit.reset();

        if (NewSelectedNetworkMapping != SelectedNetworkMapping) {
            delete pVehicleObjDetInfo;
            SelectedNetworkMapping = NewSelectedNetworkMapping;
            pVehicleObjDetInfo = new NetworkInferenceDetectionObjInfo;
            pVehicleObjDetInfo->PerformaceFPS = 0; //Just an initial value
            LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].NetworkInit(pVehicleObjDetInfo, this->m_AppID.toStdString());
            qDebug() << "Network CHANGED";
        }

        if (!m_pImageInferQueue->empty()){

            timer.start();

            ResourceLock.lock();

            AppImageData* pAppData = new AppImageData();
            pAppData->VisualizedImage = m_pImageInferQueue->front().copy();
            m_pImageInferQueue->pop_front();

            ResourceLock.unlock();

            /* First Stage
             * Vehicle Detection */

            //qDebug() << "1 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;


            LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].InferProcess(pVehicleObjDetInfo, pAppData);

            LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].ReadOutputProcess(pVehicleObjDetInfo, pAppData);


            //Here we check the vehicle that is qualified to extract license plate for inference and we crop the vehicle image
            //and save it to temporary image list for second stage (license plate detection)
            ProcessTrackerAndExtractDetectedVehiclesForLPD( pLicensePlateObjDetInfo,
                                                            pVehicleObjDetInfo,
                                                            pAppData,
                                                            LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].SupportedClassList);

            /* Second Stage
             * License Plate Detection */

            //Inference all the vehicle image that is qualified for license plate detection from the saved cropped vehicle image list        
            Yolov4TinyLicensePlate_InferWorker(pLicensePlateObjDetInfo,
                                               pAppData);

            //Here we check the result of license plate detection and check which are qualified for license plate recognition
            //and we update the vehicle tracker state to proceed into 3rd stage 
            ProcessTrackerAndPrepareDetectedVehicleWithLpdForLPR(   pLicensePlateObjDetInfo,
                                                                    pVehicleObjDetInfo,
                                                                    pAppData,
                                                                    LprObjectDetectionSupportedNetworkMappingList[SelectedNetworkMapping].SupportedClassList);

            /* Third Stage
             * License Plate Recognition */

            //Here we simply prepare the license plate image for LPR inference
            //for that we use the saved vehicle image list, crop the license plate and save it back to the same temporary list
            ProcessTrackerAndExtractDetectedLpdForLPR(  pLicensePlateObjDetInfo,
                                                        pLicensePlateRecognitionInfo, 
                                                        pAppData);

            //Here we infer the license plate image that is saved in the temporary image list
            LprNet_InferWorker(pLicensePlateRecognitionInfo, pAppData);


            //Here we check the result of license plate recognition and check which are qualified
            //and we update the vehicle tracker state to proceed into final visualization stage 
            ProcessTrackerOfLprResults( pLicensePlateRecognitionInfo,  
                                        pVehicleObjDetInfo,
                                        pAppData);


            /* Final Stage
             * Visualize */


            VisualizeResult(pVehicleObjDetInfo, pAppData);

            QImage FinalImage = pAppData->VisualizedImage;
            emit sendAppResultImage(FinalImage, QList<QGraphicsItem*>());


            //qDebug() << "8 output readed at " << timer.nsecsElapsed() << "from " << this->m_AppID;

            delete pAppData;

            //Send the signal we are done first before we sleep to limt the FPS.
            bFrameInProcess = false;

            //Limit FPS
            if (this->m_LimitFPS != 0) {
                double sleepfor = (1000.0f/this->m_LimitFPS) - TimerFPSLimit.getElapsedInMs();
                if (sleepfor > 0.0f)
                    QThread::currentThread()->msleep(sleepfor);
            }

            FrameCount++;

            if (TimerFPS.isTimePastSec(2.0)) {
                pVehicleObjDetInfo->PerformaceFPS = FrameCount/TimerFPS.getElapsedInSec();
                TimerFPS.reset();
                FrameCount = 0;
            }
        }
        else {
            QThread::currentThread()->usleep(100);
        }
    }

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    pHailoPipeline->ReleaseStreamChannel(0, pVehicleObjDetInfo->AppID);

    delete m_pImageInferQueue;
    delete pVehicleObjDetInfo;
    qDebug() << "LPR::run exiting";
}


/* Private Methods */

template <typename T>
QImage LPR::PadImage(const QImage & source, int targetWidth, int TargetHeight, T padValue)
{
    int padWidth = (targetWidth - source.width())/2;
    int padHeight = (TargetHeight - source.height())/2;

    QImage padded{targetWidth, TargetHeight, source.format()};
    padded.fill(padValue);
    QPainter p{&padded};
    p.drawImage(QPoint(padWidth, padHeight), source);
    return padded;
}



void LPR::ProcessTrackerAndExtractDetectedVehiclesForLPD(   NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo,
                                                            NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                                            AppImageData* pImageData, 
                                                            std::vector<int>    &SupportedClassList)
{

#ifdef LICENSE_PLATE_DET_TRACKER_USE


    //Update the tracker with the latest vehicle detection result
    std::vector<HailoDetectionPtr> LicensePlateDetTracker = HailoTracker::GetInstance().update(pVehicleDetectionInfo->AppID, pVehicleDetectionInfo->DecodedResult);

    for (HailoDetectionPtr  &tracker : LicensePlateDetTracker) {

        //Here we only check for supported class, otherwise we skip and continue for the next tracker
        int trackerClassId = tracker->get_class_id();
        if (std::find(SupportedClassList.begin(), SupportedClassList.end(), trackerClassId) == SupportedClassList.end())
            continue;

        auto unique_ids = hailo_common::get_hailo_unique_id(tracker);
        if (std::find(licese_plate_det_tracked_ids_temp.begin(), licese_plate_det_tracked_ids_temp.end(), unique_ids[0]->get_id()) != licese_plate_det_tracked_ids_temp.end())
        {
            // We already added usermeta to this track id, we should not add it again.
            continue;
        }
        else
        {
            licese_plate_det_tracked_ids_temp.emplace_back(unique_ids[0]->get_id());

            HailoUserMetaPtr usermeta = std::make_shared<HailoUserMeta>(HailoUserMeta(LICENSE_PLATE_DET_TRACKER_INITIAL,"Tracking",0.0f));
            HailoTracker::GetInstance().add_object_to_track(pVehicleDetectionInfo->AppID, unique_ids[0]->get_id(), usermeta);

            //WARNING:          this list will keep accumulating, during actual implementation need to clean up.
            //POSIBLE SOLUTION  Create another function(say function named A) and check all the id in licese_plate_det_tracked_ids_temp
            //                  by calling HailoTracker::GetInstance().is_object_tracked, if returned false then remove the
            //                  ids from licese_plate_det_tracked_ids_temp.
            //                  This function A is to be called by main thread every few seconds for clean up purposes

            //qDebug() << "list id size: " << licese_plate_det_tracked_ids_temp.size();
            //qDebug() << "New Tracked ID: " << unique_ids[0]->get_id();

        }
    }

    pVehicleDetectionInfo->DecodedResult = LicensePlateDetTracker; //Replace with updated one

#else

    int TestCount = 0;

    //Without tracker we will simply have to pass all face object for inference
    for (HailoDetectionPtr  &tracker : pVehicleDetectionInfo->DecodedResult) {

        //Here we only check for supported class, otherwise we skip and continue for the next tracker
        int trackerClassId = tracker->get_class_id();
        if (std::find(SupportedClassList.begin(), SupportedClassList.end(), trackerClassId) == SupportedClassList.end())
            continue;

        HailoUserMetaPtr usermeta = std::make_shared<HailoUserMeta>(HailoUserMeta(LICENSE_PLATE_DET_TRACKER_NEED_PREDICT,"Tracking",0));
        tracker->add_object(usermeta);
    }

#endif

    //Get the scale between image and object detection network input size
    float widthScale = (float)pVehicleDetectionInfo->NetworkInputWidth * (float)pVehicleDetectionInfo->scaledRatioWidth;
    float heightScale = (float)pVehicleDetectionInfo->NetworkInputHeight * (float)pVehicleDetectionInfo->scaledRatioHeight;

    //For all vehicle class we cropp the vehicle and save it to image worker list

    for (HailoDetectionPtr  &tracker : pVehicleDetectionInfo->DecodedResult) {

        //Here we only check for supported class, otherwise we skip and continue for the next tracker
        int trackerClassId = tracker->get_class_id();
        if (std::find(SupportedClassList.begin(), SupportedClassList.end(), trackerClassId) == SupportedClassList.end())
            continue;


        //Skip for vehicle that does not have metadata
        std::vector<HailoObjectPtr> ObjPtr = tracker->get_objects_typed(HAILO_USER_META);
        if (ObjPtr.size() == 0)
            continue;

        //Skip for vehicle that are already identified or no plate found
        HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);
        if  ((UserMetaPtr->get_user_int() == LICENSE_PLATE_RECOGNITION_IDENTIFIED) ||
             (UserMetaPtr->get_user_int() == LICENSE_PLATE_DET_TRACKER_NO_PLATE_FOUND))
            continue;
        
        QRect rect(tracker->get_bbox().xmin()*widthScale,
                    tracker->get_bbox().ymin()*heightScale,
                    tracker->get_bbox().width()*widthScale,
                    tracker->get_bbox().height()*heightScale);

#if 1
        //Here we use mask, a lower rectangle from the original image where car passing by within the mask will be taken
        //for license plate detection, otherwise we skip. This method will save resources as we don't want all cars in the image
        //to be infered for license plate detection.

        float x =  pImageData->VisualizedImage.rect().height() * 0.3f;
        QRect DetectionArea(0,pImageData->VisualizedImage.rect().height() - x,
                            pImageData->VisualizedImage.rect().width(), x);

        QRect intersection = DetectionArea.intersected(rect);

        float overlapPercentage = ((float)(intersection.width() * intersection.height()) / (float)(rect.width() * rect.height())) * 100.0f;
        if (overlapPercentage < 50.0f) {
            continue;
        }
#else        
        //If the rect area is at least 30% of 416x416 then we will use the whole image and set UserMetaPtr to LICENSE_PLATE_DET_TRACKER_NEED_PREDICT
        //otherwise we skip and continue
        //WARNING:  This may be adjusted for different streaming video resolution and angles of the camera
        //          The intention here is to save inference time by not infering on small vehicles but without this check 
        //          it will still work so can be safely removed if needed.
        if ((rect.width()*rect.height()) < (pLicensePlateDetInfo->NetworkInputWidth*pLicensePlateDetInfo->NetworkInputHeight*0.3)) {
            continue;
        }
#endif

        QImage cropped = pImageData->VisualizedImage.copy(rect);

        QImage scaledImage = cropped.scaled(pLicensePlateDetInfo->NetworkInputWidth, pLicensePlateDetInfo->NetworkInputHeight, Qt::KeepAspectRatio);
        scaledImage.convertToFormat(QImage::Format_RGB888);

        QImage padded = PadImage(scaledImage, pLicensePlateDetInfo->NetworkInputWidth, pLicensePlateDetInfo->NetworkInputHeight, Qt::gray);

        //Save the vehicle image
        pImageData->ImageWorkerList.push_back(padded);

        //QString FileName2("SavedImage_Infer_padded_");
        //padded.save(FileName2.append(QString::number(TestCount++)).append(".jpg"));
        
        UserMetaPtr->set_user_int(LICENSE_PLATE_DET_TRACKER_NEED_PREDICT);       
    }

}



void LPR::ProcessTrackerAndPrepareDetectedVehicleWithLpdForLPR( NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo,
                                                                NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                                                AppImageData* pImageData, 
                                                                std::vector<int>    &SupportedClassList)
{

    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    //Clear all the previous decoded result
    pLicensePlateDetInfo->DecodedResult.clear();

    for (HailoDetectionPtr  &tracker : pVehicleDetectionInfo->DecodedResult) {

        //Skip for vehicle that does not have metadata
        std::vector<HailoObjectPtr> ObjPtr = tracker->get_objects_typed(HAILO_USER_META);
        if (!ObjPtr.size())
            continue;

        //Skip for vehicle that are already identified
        HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);
        if (UserMetaPtr->get_user_int() == LICENSE_PLATE_DET_TRACKER_NEED_PREDICT) {

            //First we set back to initial state, after the prediction we will set the state appropriately
            //otherwise it will remain in initial state until the vehicle detection inference change the state to LICENSE_PLATE_DET_TRACKER_NEED_PREDICT
            UserMetaPtr->set_user_int(LICENSE_PLATE_DET_TRACKER_INITIAL);

            std::vector<HailoDetectionPtr> TempDecodedResult;
            if (Yolov4TinyLicensePlate_ReadOutputWorker(pLicensePlateDetInfo, TempDecodedResult)) {
                                
                //We did not found license plate for this vehicle
                //This logic is when there is stationary car in the camera that is unable to detect license plate
                if (TempDecodedResult.size() == 0) {

                    float NoLicensePlateCount = UserMetaPtr->get_user_float();
                    if (NoLicensePlateCount > VEHICLE_LICENSE_PLATE_NOT_FOUNT_COUNT_MAX) {
                        UserMetaPtr->set_user_int(LICENSE_PLATE_DET_TRACKER_NO_PLATE_FOUND);
                        UserMetaPtr->set_user_string("No LPD");
                    }
                    else {
                        UserMetaPtr->set_user_float(++NoLicensePlateCount);
                    }

                    //Not the image that we need to send to OCR, we remove it from the list
                    pImageData->ImageWorkerList.pop_front();

                    continue;
                }

                HailoDetectionPtr maxConfidenceTracker = nullptr;
                float             maxConfidence = 0.0f;
                for (HailoDetectionPtr  &tracker : TempDecodedResult) {
                    
                    if (tracker->get_confidence() > maxConfidence) {
                        maxConfidenceTracker = tracker;
                        maxConfidence = tracker->get_confidence();
                        //qDebug() << "Class id: " << tracker->get_class_id() << ", Conf: " << tracker->get_confidence();
                    }
                }          

                if (maxConfidenceTracker->get_confidence() >= 0.75) {

                    UserMetaPtr->set_user_int(LICENSE_PLATE_DET_TRACKER_IDENTIFIED);

                    //DEBUG PURPOSE ONLY - Save all license plate "detected" vehicles
                    //static int TestCount = 0; //This static variable will keep increasing.
                    //QString FileName2("SavedVehicleLpd_");
                    //pImageData->ImageWorkerList.front().save(FileName2.append(QString::number(TestCount++)).append(".jpg"));

                    //This is the image that we need to send to OCR, we move the top (first) to the back of the list
                    //TODO: maybe it is easier to understand (from coding point of view) to use a separate list for OCR
                    pImageData->ImageWorkerList.move(0, pImageData->ImageWorkerList.size()-1);

                    //Save the decoded result that we want to send to OCR
                    pLicensePlateDetInfo->DecodedResult.push_back(maxConfidenceTracker);
                }
                else {

                    //Not the image that we need to send to OCR, we remove it from the list
                    pImageData->ImageWorkerList.pop_front();
                }                               
            }
        }        
    }

}


void LPR::ProcessTrackerAndExtractDetectedLpdForLPR( NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, 
                                                     NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, 
                                                     AppImageData* pImageData)
{
    //Get the scale between image and object detection network input size
    float widthScale = (float)pLicensePlateDetInfo->NetworkInputWidth * (float)pLicensePlateDetInfo->scaledRatioWidth;
    float heightScale = (float)pLicensePlateDetInfo->NetworkInputHeight * (float)pLicensePlateDetInfo->scaledRatioHeight;

    for (HailoDetectionPtr  &tracker : pLicensePlateDetInfo->DecodedResult) {

        QRect rect(tracker->get_bbox().xmin()*widthScale,
                    tracker->get_bbox().ymin()*heightScale,
                    tracker->get_bbox().width()*widthScale,
                    tracker->get_bbox().height()*heightScale);

        QImage cropped = pImageData->ImageWorkerList.front().copy(rect);
        pImageData->ImageWorkerList.pop_front();

        //qDebug() << "LPR Rec: " << rect;

        QImage scaledImage = cropped.scaled(pLicensePlateRecognitionInfo->NetworkInputWidth, pLicensePlateRecognitionInfo->NetworkInputHeight, Qt::KeepAspectRatio);
        scaledImage.convertToFormat(QImage::Format_RGB888);

        QImage padded = PadImage(scaledImage, pLicensePlateRecognitionInfo->NetworkInputWidth, pLicensePlateRecognitionInfo->NetworkInputHeight, Qt::gray);

        //We push back the license plate padded image back to the image worker list
        pImageData->ImageWorkerList.push_back(padded);

        //DEBUG PURPOSE ONLY - Save all license plate "detected" vehicles
        //static int TestCount = 0; //This static variable will keep increasing.
        //QString FileName2("SavedLicensePlate_");
        //padded.save(FileName2.append(QString::number(TestCount++)).append(".jpg"));                    
    }        
}                                                     


void LPR::ProcessTrackerOfLprResults(NetworkInferenceBasedObjInfo*     pLicensePlateRecognitionInfo,  
                                     NetworkInferenceDetectionObjInfo* pVehicleDetectionInfo,
                                     AppImageData*                     pImageData)
{

    //Note that the order of pVehicleDetectionInfo tracker with state LICENSE_PLATE_DET_TRACKER_IDENTIFIED
    //is the same as the order of inference read output result of pLicensePlateRecognitionInfo.
    for (HailoDetectionPtr  &tracker : pVehicleDetectionInfo->DecodedResult) {

        //Skip for vehicle that does not have metadata
        std::vector<HailoObjectPtr> ObjPtr = tracker->get_objects_typed(HAILO_USER_META);
        if (!ObjPtr.size())
            continue;

        //We work with vehicle that has state LICENSE_PLATE_DET_TRACKER_IDENTIFIED
        HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);
        if (UserMetaPtr->get_user_int() == LICENSE_PLATE_DET_TRACKER_IDENTIFIED) {
            
            stLprResult LprResult = LprNet_ReadOutputWorker(pLicensePlateRecognitionInfo);

            if (LprResult.confidence >= 0.5)
            {

#if SAVE_LICENCE_PLATE_IMAGE
                //Save the image with file name given by LprStr
                pImageData->ImageWorkerList.front().save(QString::fromStdString(LprResult.licensePlate).append("_").append(QString::number(LprResult.confidence)).append(".jpg"));
                pImageData->ImageWorkerList.pop_front();
#endif
                //We identified the license plate number!
                UserMetaPtr->set_user_string(LprResult.licensePlate);
                UserMetaPtr->set_user_int(LICENSE_PLATE_RECOGNITION_IDENTIFIED);

            }
            else {

                //Confidence not enough, let's send this tracker back to initial state to proceed for another round of license plate recognition
                UserMetaPtr->set_user_int(LICENSE_PLATE_DET_TRACKER_INITIAL);                    
            }
        }        
    }
}                                     



void LPR::VisualizeResult(NetworkInferenceDetectionObjInfo* pVehicleDetInfo, AppImageData* pImageData)
{

    int totalDetections = pVehicleDetInfo->DecodedResult.size();
    QPainter qPainter(&pImageData->VisualizedImage);
    qPainter.setPen(QPen(Qt::red, 2));

    QFont font;
    font.setPixelSize(24);
    qPainter.setFont(font);

    float widthScale = (float)pVehicleDetInfo->NetworkInputWidth * pVehicleDetInfo->scaledRatioWidth;
    float heightScale = (float)pVehicleDetInfo->NetworkInputHeight * pVehicleDetInfo->scaledRatioHeight;

    for (int k = 0; k < totalDetections; k++){                

        std::vector<HailoObjectPtr> ObjPtr = pVehicleDetInfo->DecodedResult[k]->get_objects_typed(HAILO_USER_META);
        if (ObjPtr.size()) {
            HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);

            if (UserMetaPtr->get_user_int() == LICENSE_PLATE_RECOGNITION_IDENTIFIED) {
                qPainter.setPen(QPen(Qt::green, 2));
            }
            else if (UserMetaPtr->get_user_int() == LICENSE_PLATE_DET_TRACKER_IDENTIFIED) {
                qPainter.setPen(QPen(Qt::blue, 2));
            }
            else if (UserMetaPtr->get_user_int() == LICENSE_PLATE_DET_TRACKER_NO_PLATE_FOUND) {
                qPainter.setPen(QPen(Qt::yellow, 2));
            }
            else {
                qPainter.setPen(QPen(Qt::red, 2));
            }

            qPainter.drawText(pVehicleDetInfo->DecodedResult[k]->get_bbox().xmin()*widthScale,
                              pVehicleDetInfo->DecodedResult[k]->get_bbox().ymin()*heightScale,
                              QString::fromStdString(UserMetaPtr->get_user_string()));

            qPainter.drawRect(  pVehicleDetInfo->DecodedResult[k]->get_bbox().xmin()*widthScale,
                                pVehicleDetInfo->DecodedResult[k]->get_bbox().ymin()*heightScale,
                                pVehicleDetInfo->DecodedResult[k]->get_bbox().width()*widthScale,
                                pVehicleDetInfo->DecodedResult[k]->get_bbox().height()*heightScale);

            qPainter.setPen(QPen(Qt::red, 2));
        }

        qPainter.drawText(5,25, QString("FPS: ") + QString::number(pVehicleDetInfo->PerformaceFPS, 'g', 4));
    }

    qPainter.end();

}
