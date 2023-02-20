#include "yolov5_faceDet_process.h"

#include <QThread>
#include <QElapsedTimer>

void yolov5_person_face_print_output_result(ObjectDetectionInfo* pInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult);


int Yolov5_PersonFace_Initialize(ObjectDetectionInfo* pInitData, std::string AppID) {
    //qDebug() << "Yolov5mInitialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = std::string("yolov5PersonFace").append(AppID);
    Network.hef_path = "yolov5s_personface.hef";
    Network.output_order_by_name = std::vector<std::string>({"yolov5s_personface/conv70",
                                                             "yolov5s_personface/conv63",
                                                             "yolov5s_personface/conv55"});
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = true;
    Network.out_format = HAILO_FORMAT_TYPE_UINT8;
    pHailoPipeline->AddNetwork(0, Network, AppID);

    pInitData->ModelID = Network.id_name;
    pInitData->AppID = AppID;
    pInitData->OutputFormat = Network.out_format;
    pInitData->NetworkInputHeight = 640;
    pInitData->NetworkInputWidth = 640;
    pInitData->NetworkInputSize = pInitData->NetworkInputHeight*pInitData->NetworkInputWidth*3; //RGB channel

    pHailoPipeline->GetNetworkQuantizationInfo(pInitData->ModelID, pInitData->QuantizationInfo);

    pHailoPipeline->InitializeOutputBuffer<uint8_t>(pInitData->ModelID, pInitData->OutputBufferUint8, AppID);

    return 0;
}


void Yolov5_PersonFace_InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData) {
    //qDebug() << "Yolov5mInfer";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    //TODO: Check if we have room for perfformance improvement such as
    //      1. Faster scaling?
    //      2. Faster way to assign image raw data
    //      NOTE: We won't be able to move the ImageInputRaw to pInitData since infer could happen in parallel so it has to be independent
    //QImage scaledImage = inferImage.scaled(pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::KeepAspectRatio, Qt::FastTransformation);
    QImage scaledImage = pData->VisualizedImage.scaled(pInfo->NetworkInputWidth, pInfo->NetworkInputHeight, Qt::KeepAspectRatio);

    //qDebug() << "Scaled image size in byte: " << scaledImage.sizeInBytes();
    //qDebug() << "Scaled image size: " << scaledImage.width() << ", " << scaledImage.height();
    //qDebug() << "original image size: " << pData->VisualizedImage.width() << ", " << pData->VisualizedImage.height();
    //qDebug() << "ImageInpuitRaw buffer size: " <<pInfo->ImageInputRaw.size();

    //TODO: Should not need to calculate this every time, only when resolution changes, move to more appropriate place
    pInfo->scaledRatioWidth = pData->VisualizedImage.width() / scaledImage.width();
    pInfo->scaledRatioHeight = pData->VisualizedImage.height() / scaledImage.height();

    const uchar* pImagedata = scaledImage.bits();

    //NOTE: We need the resize after assign because
    //      1. the scaled image aspect ratio fit size in byte could be smaller than input network size as it will not pad
    //      2. the assign automatically rescale base on the total item it is assigned
    //      3. so we need to resize to add/pad the remaining item (and set it to 0) to make it the same as network input size.
    pInfo->ImageInputRaw.assign(pImagedata, pImagedata+scaledImage.sizeInBytes());
    pInfo->ImageInputRaw.resize(pInfo->NetworkInputSize, 0);


    pHailoPipeline->Infer(pInfo->ModelID, pInfo->ImageInputRaw, pInfo->AppID);

}


void Yolov5_PersonFace_ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData) {

    //qDebug() << "Yolov5mReadOutput";

    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    QElapsedTimer timer;

    if (pInfo->OutputFormat == HAILO_FORMAT_TYPE_UINT8)
        ReadOutRet = pHailoPipeline->ReadOutputById(pInfo->ModelID, pInfo->OutputBufferUint8, pInfo->AppID);
    else //Must be float 32
        qDebug() << "WARNING: Output HAILO_FORMAT_TYPE_FLOAT32 NOT YET SUPPORTED";

    if (ReadOutRet == MnpReturnCode::SUCCESS) {

        //timer.start();

        pData->DecodedResult = Yolov5PersonFaceDecode(pInfo, pInfo->OutputBufferUint8);
        //qDebug() << "output readed at " << timer.nsecsElapsed();

    }
}

void Yolov5_PersonFace_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData) {

    //qDebug() << "Yolov5mVisualize";

    int totalDetections = pData->DecodedResult.size();
    QPainter qPainter(&pData->VisualizedImage);
    qPainter.setPen(QPen(Qt::red, 2));

    QFont font;
    font.setPixelSize(24);
    qPainter.setFont(font);

    float widthScale = pInfo->NetworkInputWidth * pInfo->scaledRatioWidth;
    float heightScale = pInfo->NetworkInputHeight * pInfo->scaledRatioHeight;

    for (int k = 0; k < totalDetections; k++){
        //We ignore all prediction is provability smaller than 50%
        if (pData->DecodedResult[k]->get_confidence() < 0.4)
            continue;

        qPainter.drawRect(  pData->DecodedResult[k]->get_bbox().xmin()*widthScale,
                            pData->DecodedResult[k]->get_bbox().ymin()*heightScale,
                            pData->DecodedResult[k]->get_bbox().width()*widthScale,
                            pData->DecodedResult[k]->get_bbox().height()*heightScale);

        qPainter.drawText(5,25, QString("FPS: ") + QString::number(pInfo->PerformaceFPS, 'g', 4));

    }

    qPainter.end();

    //yolov5_person_face_print_output_result(pInfo, totalDetections, pData->DecodedResult);

}


std::vector<HailoDetectionPtr> Yolov5PersonFaceDecode(ObjectDetectionInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode) {
    //qDebug() << "Yolov5PersonFaceDecode";

    static bool Initialized = false;
    static Yolov5NmsDecoder<uint8_t> Yolov5Decoder(true);
    static std::vector<int> anchors1 {116, 90, 156, 198, 373, 326};
    static std::vector<int> anchors2 {30,  61, 62,  45,  59,  119};
    static std::vector<int> anchors3 {10,  13, 16,  30,  33,  23};

    if (Initialized == false) {

        QunatizationInfo quantInfo;
        Yolov5Decoder.YoloConfig(pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, YOLO_PERSON_FACE_TOTAL_CLASS, YOLO_PERSON_FACE_CONFIDENCE_THRS);

        quantInfo.qp_scale = pInitData->QuantizationInfo[0].qp_scale;
        quantInfo.qp_zp = pInitData->QuantizationInfo[0].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLO_PERSON_FACE_FEATURE_MAP_SIZE1, YOLO_PERSON_FACE_FEATURE_MAP_SIZE1, anchors1, &quantInfo);

        quantInfo.qp_scale = pInitData->QuantizationInfo[1].qp_scale;
        quantInfo.qp_zp = pInitData->QuantizationInfo[1].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLO_PERSON_FACE_FEATURE_MAP_SIZE2, YOLO_PERSON_FACE_FEATURE_MAP_SIZE2, anchors2, &quantInfo);

        quantInfo.qp_scale = pInitData->QuantizationInfo[2].qp_scale;
        quantInfo.qp_zp = pInitData->QuantizationInfo[2].qp_zp;
        Yolov5Decoder.YoloAddOutput(YOLO_PERSON_FACE_FEATURE_MAP_SIZE3, YOLO_PERSON_FACE_FEATURE_MAP_SIZE3, anchors3, &quantInfo);

        Initialized = true;
    }

    return Yolov5Decoder.decode(OutputForDecode[0], OutputForDecode[1], OutputForDecode[2]);
}


std::string yolov5_person_face_get_classname(int cls)
{
    std::string result = "N/A";
    switch(cls) {
        case 0: result = "__background__";break;
        case YOLO_PERSONFACE_PERSON_CLASS_ID: result = "person";break;
        case YOLO_PERSONFACE_FACE_CLASS_ID: result = "face";break;
    }
    return result;
}

void yolov5_person_face_print_output_result(ObjectDetectionInfo* pInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult)
{

    QDebug debug1 = qDebug();
    //For each detection we show its detail as output in text as well as visualization
    if (total_detection > 0) {
        debug1 << "-I- Num detections: " << total_detection << " Classes: [";
        for (size_t i = 0; i < total_detection;i++)
            debug1 << yolov5_person_face_get_classname(detectionsResult[i]->get_class_id()).data() << " ";
        debug1 << "]";

        //Show Detail Text Outputs
        //Each detection is 6 bytes with the following mapping arrangement
        //[Y-Start, X-Start, Y-End, X-End, Class, Provability]
        //NOTE: X and Y is normalized between 0 ~ 1, to get the image coordinate multiply by input size
        qDebug() << "-I- [Y-Start, X-Start, Y-End, X-End, Class, Provability]";

        for (size_t k = 0; k < total_detection; k++){
            QDebug debug2 = qDebug();
            debug2 << "-I- Detection Data: [";
            debug2 << detectionsResult[k]->get_bbox().ymin() * pInfo->NetworkInputHeight << " ";
            debug2 << detectionsResult[k]->get_bbox().xmin() * pInfo->NetworkInputWidth << " ";
            debug2 << detectionsResult[k]->get_bbox().ymax() * pInfo->NetworkInputHeight << " ";
            debug2 << detectionsResult[k]->get_bbox().xmax() * pInfo->NetworkInputWidth << " ";
            debug2 << detectionsResult[k]->get_class_id() << " ";
            debug2 << detectionsResult[k]->get_confidence() << " ";

            if (detectionsResult[k]->get_confidence() < 0.5)
                debug2 << "] - LOW Provability prediction (less than 50%)";
            else
                debug2 << "]";
        }
    }

}

int Yolov5_PersonFace_ShareDataCleanUp(ObjectDetectionData* pShareData) {
    //qDebug() << "Yolov5mShareDataCleanUp";

    delete pShareData;
    return 0;
}
