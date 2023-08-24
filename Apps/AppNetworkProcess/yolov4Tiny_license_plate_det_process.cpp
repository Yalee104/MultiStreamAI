#include "yolov4Tiny_license_plate_det_process.h"
#include <opencv2/opencv.hpp>

#include <QThread>
#include <QElapsedTimer>
#include <QDir>

//TODO: Revise the use of define and use more structurized design pattern
#define OPENCV_RESIZE

int Yolov4TinyLicensePlate_Initialize(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, std::string AppID) {
    //qDebug() << "Yolov4TinyLicensePlate_Initialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = std::string("Yolov4TinyLicensePlateDet").append(AppID);
    Network.hef_path = "tiny_yolov4_license_plates.hef";
    Network.output_order_by_name = std::vector<std::string>({"tiny_yolov4_license_plates/conv19",
                                                             "tiny_yolov4_license_plates/conv21"});
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;
    pHailoPipeline->AddNetwork(0, Network, AppID);

    pLicensePlateDetInfo->ModelID = Network.id_name;
    pLicensePlateDetInfo->AppID = AppID;
    pLicensePlateDetInfo->OutputFormat = Network.out_format;
    pLicensePlateDetInfo->NetworkInputHeight = 416;
    pLicensePlateDetInfo->NetworkInputWidth = 416;
    pLicensePlateDetInfo->NetworkInputSize = pLicensePlateDetInfo->NetworkInputHeight*pLicensePlateDetInfo->NetworkInputWidth*3; //RGB channel

    //The scale ration is one to one since we are using the same input image
    pLicensePlateDetInfo->scaledRatioWidth = 1.0f; 
    pLicensePlateDetInfo->scaledRatioHeight = 1.0f;

    pHailoPipeline->GetNetworkQuantizationInfo(pLicensePlateDetInfo->ModelID, pLicensePlateDetInfo->QuantizationInfo);

    pHailoPipeline->InitializeOutputBuffer<float32_t>(pLicensePlateDetInfo->ModelID, pLicensePlateDetInfo->OutputBufferFloat32, AppID);

#ifdef LICENSE_PLATE_DET_TRACKER_USE
    HailoTracker::GetInstance().add_jde_tracker(pLicensePlateDetInfo->AppID);
    //We want to keep past metadata since we will be adding user specific meta data
    HailoTracker::GetInstance().set_keep_past_metadata(pLicensePlateDetInfo->AppID, true);
#endif

    return 0;
}

void Yolov4TinyLicensePlate_InferWorker(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo,
                                        AppImageData* pImageData)
{
    //qDebug() << "Yolov4TinyLicensePlate_InferWorker";
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    for (QImage &image : pImageData->ImageWorkerList) {
        const uchar* pImageVehicle = image.bits();
        //const uchar* pImageVehicle = scaledImage.bits();

        pLicensePlateDetInfo->ImageInputRaw.assign(pImageVehicle, pImageVehicle+image.sizeInBytes());
        //pLicensePlateDetInfo->ImageInputRaw.resize(pLicensePlateDetInfo->NetworkInputSize, 0);
        //qDebug() << "pLicensePlateDetInfo->ImageInputRaw size: " << pLicensePlateDetInfo->ImageInputRaw.size();
        pHailoPipeline->Infer(pLicensePlateDetInfo->ModelID, pLicensePlateDetInfo->ImageInputRaw, pLicensePlateDetInfo->AppID); 

    }    

    //qDebug() << "Yolov4TinyLicensePlate_InferWorker exit";
}

std::vector<HailoDetectionPtr> Yolov4TinyLicensePlate_Decode(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, std::vector<std::vector<float32_t>> &OutputForDecode) {

    //qDebug() << "Yolov4TinyLicensePlate_Decode";
    
    static bool Initialized = false;

    static Yolov4NmsDecoder<float> Yolov4Decoder(false);
    static std::vector<int> anchors1 {81, 82, 135, 169, 344, 319};
    static std::vector<int> anchors2 {10, 14, 23, 27, 37, 58};

    if (Initialized == false) {

        QunatizationInfo quantInfo;
        Yolov4Decoder.YoloConfig(pLicensePlateDetInfo->NetworkInputWidth, pLicensePlateDetInfo->NetworkInputHeight, YOLOV4_TINY_LPD_TOTAL_CLASS, YOLOV4_TINY_LPD_CONFIDENCE_THRS);

        quantInfo.qp_scale = pLicensePlateDetInfo->QuantizationInfo[0].qp_scale;
        quantInfo.qp_zp = pLicensePlateDetInfo->QuantizationInfo[0].qp_zp;
        Yolov4Decoder.YoloAddOutput(YOLOV4_TINY_LPD_FEATURE_MAP_SIZE1, YOLOV4_TINY_LPD_FEATURE_MAP_SIZE1, anchors1, &quantInfo);

        quantInfo.qp_scale = pLicensePlateDetInfo->QuantizationInfo[1].qp_scale;
        quantInfo.qp_zp = pLicensePlateDetInfo->QuantizationInfo[1].qp_zp;
        Yolov4Decoder.YoloAddOutput(YOLOV4_TINY_LPD_FEATURE_MAP_SIZE2, YOLOV4_TINY_LPD_FEATURE_MAP_SIZE2, anchors2, &quantInfo);

        Initialized = true;
    }

    std::vector<float32_t> v(0);
    return Yolov4Decoder.decode(OutputForDecode[0], OutputForDecode[1], v);
}


bool Yolov4TinyLicensePlate_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, std::vector<HailoDetectionPtr> &LpdDecodedResults)
{
 
    //qDebug() << "Yolov4TinyLicensePlate_ReadOutputWorker";    

    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    ReadOutRet = pHailoPipeline->ReadOutputById(pLicensePlateDetInfo->ModelID, pLicensePlateDetInfo->OutputBufferFloat32, pLicensePlateDetInfo->AppID);

    if (ReadOutRet == MnpReturnCode::SUCCESS) {        
        LpdDecodedResults = Yolov4TinyLicensePlate_Decode(pLicensePlateDetInfo, pLicensePlateDetInfo->OutputBufferFloat32);
        return true;
    }
    
    //qDebug() << "Yolov4TinyLicensePlate_ReadOutputWorker exit";

    return false;

}

