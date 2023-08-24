#ifndef YOLOV7_PROCESS_H
#define YOLOV7_PROCESS_H

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include "Utils/yolo-nms-decoder/yolo_nms_decoder.hpp"
#include "Apps/appcommon.h"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQueue>
#include <QPainter>
#include <QDebug>
#include <QThread>

constexpr int YOLOV7_FEATURE_MAP_SIZE1 = 20;
constexpr int YOLOV7_FEATURE_MAP_SIZE2 = 40;
constexpr int YOLOV7_FEATURE_MAP_SIZE3 = 80;
constexpr int YOLOV7_TOTAL_CLASS = 80;
constexpr float YOLOV7_CONFIDENCE_THRS = 0.5;

int Yolov7_Initialize(NetworkInferenceDetectionObjInfo* pInitData, std::string AppID);

std::vector<HailoDetectionPtr> Yolov7_Decode(NetworkInferenceDetectionObjInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov7_ShareDataCleanUp(AppImageData* pShareData);


void Yolov7_InferWorker(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);

void Yolov7_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);

void Yolov7_VisualizeWorker(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);


#endif // YOLOV7_PROCESS_H
