#include "yolov7tiny_process.h"

#include <QThread>
#include <QElapsedTimer>

void yolov7Tiny_print_output_result(NetworkInferenceDetectionObjInfo* pInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult);

int Yolov7Tiny_Initialize(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::string AppID) {
    //qDebug() << "Yolov5mInitialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = std::string("yolov7").append(AppID);
    Network.hef_path = "yolov7_tiny_500fps.hef";
    Network.output_order_by_name = std::vector<std::string>({"yolov7_tiny/conv58", "yolov7_tiny/conv51", "yolov7_tiny/conv43"});
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = true;
    Network.out_format = HAILO_FORMAT_TYPE_UINT8;
    pHailoPipeline->AddNetwork(0, Network, AppID);

    pVehicleObjInfo->ModelID = Network.id_name;
    pVehicleObjInfo->AppID = AppID;
    pVehicleObjInfo->OutputFormat = Network.out_format;
    pVehicleObjInfo->NetworkInputHeight = 640;
    pVehicleObjInfo->NetworkInputWidth = 640;
    pVehicleObjInfo->NetworkInputSize = pVehicleObjInfo->NetworkInputHeight*pVehicleObjInfo->NetworkInputWidth*3; //RGB channel

    pHailoPipeline->GetNetworkQuantizationInfo(pVehicleObjInfo->ModelID, pVehicleObjInfo->QuantizationInfo);

    pHailoPipeline->InitializeOutputBuffer<uint8_t>(pVehicleObjInfo->ModelID, pVehicleObjInfo->OutputBufferUint8, AppID);

    return 0;
}


void Yolov7Tiny_InferWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData) {
    //qDebug() << "Yolov5mInfer";

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


void Yolov7Tiny_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData)
{


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

        pVehicleObjInfo->DecodedResult = Yolov7Tiny_Decode(pVehicleObjInfo, pVehicleObjInfo->OutputBufferUint8);
        //qDebug() << "output readed at " << timer.nsecsElapsed();
        //qDebug() << "decoded result: " << pVehicleObjInfo->DecodedResult.size();
    }
}


void Yolov7Tiny_VisualizeWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData)
{

    //qDebug() << "Yolov7Tiny_VisualizeWorker";

    int totalDetections = pVehicleObjInfo->DecodedResult.size();
    QPainter qPainter(&pImageData->VisualizedImage);
    qPainter.setPen(QPen(Qt::red, 2));

    QFont font;
    font.setPixelSize(24);
    qPainter.setFont(font);

    float widthScale = (float)pVehicleObjInfo->NetworkInputWidth * pVehicleObjInfo->scaledRatioWidth;
    float heightScale = (float)pVehicleObjInfo->NetworkInputHeight * pVehicleObjInfo->scaledRatioHeight;

    for (int k = 0; k < totalDetections; k++){
        //We ignore all prediction is provability smaller than 50%
        if (pVehicleObjInfo->DecodedResult[k]->get_confidence() < 0.4)
            continue;

        qPainter.drawRect(  pVehicleObjInfo->DecodedResult[k]->get_bbox().xmin()*widthScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().ymin()*heightScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().width()*widthScale,
                            pVehicleObjInfo->DecodedResult[k]->get_bbox().height()*heightScale);

        qPainter.drawText(5,25, QString("FPS: ") + QString::number(pVehicleObjInfo->PerformaceFPS, 'g', 4));
    }

    qPainter.end();

    //yolov7Tiny_print_output_result(pVehicleObjInfo, totalDetections, pVehicleObjInfo->DecodedResult);

}


std::vector<HailoDetectionPtr> Yolov7Tiny_Decode(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::vector<std::vector<uint8_t>> &OutputForDecode)
{
    //qDebug() << "Yolov5mDecode";

    static bool Initialized = false;
    static Yolov7NmsDecoder<uint8_t> Yolov7Decoder(true);
    static std::vector<int> anchors1 {142, 110, 192, 243, 459, 401};
    static std::vector<int> anchors2 {36,  75, 76,  55,  72,  146};
    static std::vector<int> anchors3 {12,  16, 19,  36,  40,  28};

    if (Initialized == false) {

        QunatizationInfo quantInfo;
        Yolov7Decoder.YoloConfig(pVehicleObjInfo->NetworkInputWidth, pVehicleObjInfo->NetworkInputHeight, YOLOV7_TINY_TOTAL_CLASS, YOLOV7_TINY_CONFIDENCE_THRS);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[0].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[0].qp_zp;
        Yolov7Decoder.YoloAddOutput(YOLOV7_TINY_FEATURE_MAP_SIZE1, YOLOV7_TINY_FEATURE_MAP_SIZE1, anchors1, &quantInfo);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[1].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[1].qp_zp;
        Yolov7Decoder.YoloAddOutput(YOLOV7_TINY_FEATURE_MAP_SIZE2, YOLOV7_TINY_FEATURE_MAP_SIZE2, anchors2, &quantInfo);

        quantInfo.qp_scale = pVehicleObjInfo->QuantizationInfo[2].qp_scale;
        quantInfo.qp_zp = pVehicleObjInfo->QuantizationInfo[2].qp_zp;
        Yolov7Decoder.YoloAddOutput(YOLOV7_TINY_FEATURE_MAP_SIZE3, YOLOV7_TINY_FEATURE_MAP_SIZE3, anchors3, &quantInfo);

        Initialized = true;
    }

    return Yolov7Decoder.decode(OutputForDecode[0], OutputForDecode[1], OutputForDecode[2]);
}


std::string yolov7tiny_get_coco_name_from_int(int cls)
{
    std::string result = "N/A";
    switch(cls) {
        case 0: result = "__background__";break;
        case 1: result = "person";break;
        case 2: result = "bicycle";break;
        case 3: result = "car";break;
        case 4: result = "motorcycle";break;
        case 5: result = "airplane";break;
        case 6: result = "bus";break;
        case 7: result = "train";break;
        case 8: result = "truck";break;
        case 9: result = "boat";break;
        case 10: result = "traffic light";break;
        case 11: result = "fire hydrant";break;
        case 12: result = "stop sign";break;
        case 13: result = "parking meter";break;
        case 14: result = "bench";break;
        case 15: result = "bird";break;
        case 16: result = "cat";break;
        case 17: result = "dog";break;
        case 18: result = "horse";break;
        case 19: result = "sheep";break;
        case 20: result = "cow";break;
        case 21: result = "elephant";break;
        case 22: result = "bear";break;
        case 23: result = "zebra";break;
        case 24: result = "giraffe";break;
        case 25: result = "backpack";break;
        case 26: result = "umbrella";break;
        case 27: result = "handbag";break;
        case 28: result = "tie";break;
        case 29: result = "suitcase";break;
        case 30: result = "frisbee";break;
        case 31: result = "skis";break;
        case 32: result = "snowboard";break;
        case 33: result = "sports ball";break;
        case 34: result = "kite";break;
        case 35: result = "baseball bat";break;
        case 36: result = "baseball glove";break;;
        case 37: result = "skateboard";break;
        case 38: result = "surfboard";break;
        case 39: result = "tennis racket";break;
        case 40: result = "bottle";break;
        case 41: result = "wine glass";break;
        case 42: result = "cup";break;
        case 43: result = "fork";break;
        case 44: result = "knife";break;
        case 45: result = "spoon";break;
        case 46: result = "bowl";break;
        case 47: result = "banana";break;
        case 48: result = "apple";break;
        case 49: result = "sandwich";break;
        case 50: result = "orange";break;
        case 51: result = "broccoli";break;
        case 52: result = "carrot";break;
        case 53: result = "hot dog";break;
        case 54: result = "pizza";break;
        case 55: result = "donut";break;
        case 56: result = "cake";break;
        case 57: result = "chair";break;
        case 58: result = "couch";break;
        case 59: result = "potted plant";break;
        case 60: result = "bed";break;
        case 61: result = "dining table";break;
        case 62: result = "toilet";break;
        case 63: result = "tv";break;
        case 64: result = "laptop";break;
        case 65: result = "mouse";break;
        case 66: result = "remote";break;
        case 67: result = "keyboard";break;
        case 68: result = "cell phone";break;
        case 69: result = "microwave";break;
        case 70: result = "oven";break;
        case 71: result = "toaster";break;
        case 72: result = "sink";break;
        case 73: result = "refrigerator";break;
        case 74: result = "book";break;
        case 75: result = "clock";break;
        case 76: result = "vase";break;
        case 77: result = "scissors";break;
        case 78: result = "teddy bear";break;
        case 79: result = "hair drier";break;
        case 80: result = "toothbrush";break;
    }
    return result;
}

void yolov7Tiny_print_output_result(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, size_t total_detection, std::vector<HailoDetectionPtr> &detectionsResult)
{

    QDebug debug1 = qDebug();
    //For each detection we show its detail as output in text as well as visualization
    if (total_detection > 0) {
        debug1 << "-I- Num detections: " << total_detection << " Classes: [";
        for (size_t i = 0; i < total_detection;i++)
            debug1 << yolov7tiny_get_coco_name_from_int(detectionsResult[i]->get_class_id()).data() << " ";
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

int Yolov7Tiny_ShareDataCleanUp(AppImageData* pImageData)
{
    //qDebug() << "Yolov7Tiny_ShareDataCleanUp";

    delete pImageData;
    return 0;
}
