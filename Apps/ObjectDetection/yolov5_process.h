#ifndef YOLOV5_PROCESS_H
#define YOLOV5_PROCESS_H

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <hailo/hailort.h>
#include "MultiNetworkPipeline.hpp"
#include "Utils/yolo-nms-decoder/yolo_nms_decoder.hpp"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQueue>
#include <QPainter>
#include <QDebug>
#include <QThread>

constexpr int FEATURE_MAP_SIZE1 = 20;
constexpr int FEATURE_MAP_SIZE2 = 40;
constexpr int FEATURE_MAP_SIZE3 = 80;
constexpr int TOTAL_CLASS = 80;

struct ObjectDetectionInfo {
    std::string                 ModelID;
    std::string                 AppID;
    int                         NetworkInputHeight;
    int                         NetworkInputWidth;
    int                         NetworkInputSize;
    hailo_format_type_t         OutputFormat;
    std::vector<qp_zp_scale_t>  QuantizationInfo;
    float                       scaledRatioWidth;
    float                       scaledRatioHeight;
    float                       PerformaceFPS;

    std::vector<uint8_t>                ImageInputRaw;
    std::vector<std::vector<float32_t>> OutputBufferFloat32;
    std::vector<std::vector<uint8_t>>   OutputBufferUint8;

};

struct ObjectDetectionData {

    std::vector<float32_t>              DecodedResult;
    int                                 TotalPrediction;
    QImage                              VisualizedImage;

};

int Yolov5mInitialize(ObjectDetectionInfo* pInitData, std::string AppID);

std::vector<float32_t> Yolov5mDecode(ObjectDetectionInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov5mShareDataCleanUp(ObjectDetectionData* pShareData);


void InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);


#endif // YOLOV5_PROCESS_H
