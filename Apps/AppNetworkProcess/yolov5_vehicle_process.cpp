#include "yolov5_vehicle_process.h"

#include <QThread>
#include <QElapsedTimer>

void yolov5m_vehicle_print_output_result(NetworkInferenceDetectionObjInfo* pInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult);


int Yolov5m_Vehicle_Initialize(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::string AppID)
{
    //qDebug() << "Yolov5m_vehicle_Initialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = std::string("yolov5m_vehicle").append(AppID);
    Network.hef_path = "yolov5m_vehicles_640x640.hef";
    Network.output_order_by_name = std::vector<std::string>({"yolov5m_vehicles/conv94", 
                                                             "yolov5m_vehicles/conv85",
                                                             "yolov5m_vehicles/conv75"});
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = true;
    Network.out_format = HAILO_FORMAT_TYPE_UINT8;
    pHailoPipeline->AddNetwork(0, Network, AppID);

    pVehicleObjInfo->ModelID = Network.id_name;
    pVehicleObjInfo->AppID = AppID;
    pVehicleObjInfo->OutputFormat = Network.out_format;
    pVehicleObjInfo->NetworkInputHeight = 640;//1080;
    pVehicleObjInfo->NetworkInputWidth = 640;//1920;
    pVehicleObjInfo->NetworkInputSize = pVehicleObjInfo->NetworkInputHeight*pVehicleObjInfo->NetworkInputWidth*3; //RGB channel

    pHailoPipeline->GetNetworkQuantizationInfo(pVehicleObjInfo->ModelID, pVehicleObjInfo->QuantizationInfo);

    pHailoPipeline->InitializeOutputBuffer<uint8_t>(pVehicleObjInfo->ModelID, pVehicleObjInfo->OutputBufferUint8, AppID);

    return 0;
}


void Yolov5m_Vehicle_InferWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData) {
    //qDebug() << "Yolov5m_Vehicle_InferWorker";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    //TODO: Check if we have room for perfformance improvement such as
    //      1. Faster scaling?
    //      2. Faster way to assign image raw data
    //      NOTE: We won't be able to move the ImageInputRaw to pInitData since infer could happen in parallel so it has to be independent
    //QImage scaledImage = inferImage.scaled(pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
    QImage scaledImage = pImageData->VisualizedImage.scaled(pVehicleObjInfo->NetworkInputWidth, pVehicleObjInfo->NetworkInputHeight, Qt::KeepAspectRatio);

    //qDebug() << "Scaled image size in byte: " << scaledImage.sizeInBytes();
    //qDebug() << "Scaled image size: " << scaledImage.width() << ", " << scaledImage.height();
    //qDebug() << "original image size: " << pImageData->VisualizedImage.width() << ", " << pImageData->VisualizedImage.height();
    //qDebug() << "ImageInpuitRaw buffer size: " <<pVehicleObjInfo->ImageInputRaw.size();

    //TODO: Should not need to calculate this every time, only when resolution changes, move to more appropriate place
    pVehicleObjInfo->scaledRatioWidth = (float)pImageData->VisualizedImage.width() / (float)scaledImage.width();
    pVehicleObjInfo->scaledRatioHeight = (float)pImageData->VisualizedImage.height() / (float)scaledImage.height();

    const uchar* pImagedata = scaledImage.bits();

    //NOTE: We need the resize after assign because
    //      1. the scaled image aspect ratio fit size in byte could be smaller than input network size as it will not pad
    //      2. the assign automatically rescale base on the total item it is assigned
    //      3. so we need to resize to add/pad the remaining item (and set it to 0) to make it the same as network input size.
    pVehicleObjInfo->ImageInputRaw.assign(pImagedata, pImagedata+scaledImage.sizeInBytes());
    pVehicleObjInfo->ImageInputRaw.resize(pVehicleObjInfo->NetworkInputSize, 0);


    pHailoPipeline->Infer(pVehicleObjInfo->ModelID, pVehicleObjInfo->ImageInputRaw, pVehicleObjInfo->AppID);

}


void Yolov5m_Vehicle_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData) {

    //qDebug() << "Yolov5m_Vehicle_ReadOutputWorker";

    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    QElapsedTimer timer;

    if (pVehicleObjInfo->OutputFormat == HAILO_FORMAT_TYPE_UINT8)
        ReadOutRet = pHailoPipeline->ReadOutputById(pVehicleObjInfo->ModelID, pVehicleObjInfo->OutputBufferUint8, pVehicleObjInfo->AppID);
    else //Must be float 32
        qDebug() << "WARNING: Output HAILO_FORMAT_TYPE_FLOAT32 NOT YET SUPPORTED";

    if (ReadOutRet == MnpReturnCode::SUCCESS) {

        //timer.start();

        pVehicleObjInfo->DecodedResult = Yolov5m_Vehicle_Decode(pVehicleObjInfo, pVehicleObjInfo->OutputBufferUint8);
        //qDebug() << "output readed at " << timer.nsecsElapsed();
        //qDebug() << "decoded result: " << pVehicleObjInfo->DecodedResult.size();
    }
}

void Yolov5m_Vehicle_VisualizeWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData) {

    //qDebug() << "Yolov5m_Vehicle_VisualizeWorker";

    int totalDetections = pVehicleObjInfo->DecodedResult.size();
    QPainter qPainter(&pImageData->VisualizedImage);
    qPainter.setPen(QPen(Qt::red, 2));

    QFont font;
    font.setPixelSize(24);
    qPainter.setFont(font);

    float widthScale = (float)pVehicleObjInfo->NetworkInputWidth * pVehicleObjInfo->scaledRatioWidth;
    float heightScale = (float)pVehicleObjInfo->NetworkInputHeight * pVehicleObjInfo->scaledRatioHeight;

    for (int k = 0; k < totalDetections; k++){
        //We ignore all prediction is provability smaller than 70%
        if (pVehicleObjInfo->DecodedResult[k]->get_confidence() < 0.75)
            continue;

        qPainter.drawRect(  pVehicleObjInfo->DecodedResult[k]->get_bbox().xmin()*widthScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().ymin()*heightScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().width()*widthScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().height()*heightScale);

        qPainter.drawText(5,25, QString("FPS: ") + QString::number(pVehicleObjInfo->PerformaceFPS, 'g', 4));
    }

    qPainter.end();

    //yolov5m_vehicle_print_output_result(pVehicleObjInfo, totalDetections, pVehicleObjInfo->DecodedResult);

}


std::vector<HailoDetectionPtr> Yolov5m_Vehicle_Decode(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::vector<std::vector<uint8_t>> &OutputForDecode) {
    //qDebug() << "Yolov5m_Vehicle_Decode";

    static bool Initialized = false;
    static Yolov5NmsDecoder<uint8_t> Yolov5Decoder(true);
    static std::vector<int> anchors1 {116, 90, 156, 198, 373, 326};
    static std::vector<int> anchors2 {30,  61, 62,  45,  59,  119};
    static std::vector<int> anchors3 {10,  13, 16,  30,  33,  23};

    if (Initialized == false) {

        QunatizationInfo quantInfo;
        Yolov5Decoder.YoloConfig(pVehicleObjInfo->NetworkInputWidth, pVehicleObjInfo->NetworkInputHeight, YOLOV5_VEHICLE_TOTAL_CLASS, YOLOV5_VEHICLE_CONFIDENCE_THRS);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[0].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[0].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLOV5_VEHICLE_FEATURE_MAP_SIZE1, YOLOV5_VEHICLE_FEATURE_MAP_SIZE1, anchors1, &quantInfo);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[1].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[1].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLOV5_VEHICLE_FEATURE_MAP_SIZE2, YOLOV5_VEHICLE_FEATURE_MAP_SIZE2, anchors2, &quantInfo);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[2].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[2].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLOV5_VEHICLE_FEATURE_MAP_SIZE3, YOLOV5_VEHICLE_FEATURE_MAP_SIZE3, anchors3, &quantInfo);

        Initialized = true;
    }

    return Yolov5Decoder.decode(OutputForDecode[0], OutputForDecode[1], OutputForDecode[2]);
}


std::string yolov5m_vehicle_get_coco_name_from_int(int cls)
{
    std::string result = "N/A";
    switch(cls) {
        case 0: result = "__background__";break;
        case 1: result = "Car";break;
    }
    return result;
}

void yolov5m_vehicle_print_output_result(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult)
{

    QDebug debug1 = qDebug();
    //For each detection we show its detail as output in text as well as visualization
    if (total_detection > 0) {
        debug1 << "-I- Num detections: " << total_detection << " Classes: [";
        for (size_t i = 0; i < total_detection;i++)
            debug1 << yolov5m_vehicle_get_coco_name_from_int(detectionsResult[i]->get_class_id()).data() << " ";
        debug1 << "]";

        //Show Detail Text Outputs
        //Each detection is 6 bytes with the following mapping arrangement
        //[Y-Start, X-Start, Y-End, X-End, Class, Provability]
        //NOTE: X and Y is normalized between 0 ~ 1, to get the image coordinate multiply by input size
        qDebug() << "-I- [Y-Start, X-Start, Y-End, X-End, Class, Provability]";

        for (size_t k = 0; k < total_detection; k++){
            QDebug debug2 = qDebug();
            debug2 << "-I- Detection Data: [";
            debug2 << detectionsResult[k]->get_bbox().ymin() * pVehicleObjInfo->NetworkInputHeight << " ";
            debug2 << detectionsResult[k]->get_bbox().xmin() * pVehicleObjInfo->NetworkInputWidth << " ";
            debug2 << detectionsResult[k]->get_bbox().ymax() * pVehicleObjInfo->NetworkInputHeight << " ";
            debug2 << detectionsResult[k]->get_bbox().xmax() * pVehicleObjInfo->NetworkInputWidth << " ";
            debug2 << detectionsResult[k]->get_class_id() << " ";
            debug2 << detectionsResult[k]->get_confidence() << " ";

            if (detectionsResult[k]->get_confidence() < 0.5)
                debug2 << "] - LOW Provability prediction (less than 50%)";
            else
                debug2 << "]";
        }
    }

}

int Yolov5m_Vehicle_ShareDataCleanUp(AppImageData* pImageData) {
    //qDebug() << "Yolov5m_Vehicle_ShareDataCleanUp";

    delete pImageData;
    return 0;
}
