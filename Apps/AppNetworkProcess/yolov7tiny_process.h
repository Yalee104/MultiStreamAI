#ifndef YOLOV7TINY_PROCESS_H
#define YOLOV7TINY_PROCESS_H

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

constexpr int YOLOV7_TINY_FEATURE_MAP_SIZE1 = 20;
constexpr int YOLOV7_TINY_FEATURE_MAP_SIZE2 = 40;
constexpr int YOLOV7_TINY_FEATURE_MAP_SIZE3 = 80;
constexpr int YOLOV7_TINY_TOTAL_CLASS = 80;
constexpr float YOLOV7_TINY_CONFIDENCE_THRS = 0.5;

int Yolov7Tiny_Initialize(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::string AppID);

std::vector<HailoDetectionPtr> Yolov7Tiny_Decode(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov7Tiny_ShareDataCleanUp(AppImageData* pImageData);


void Yolov7Tiny_InferWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);

void Yolov7Tiny_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);

void Yolov7Tiny_VisualizeWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);


#endif // YOLOV7TINY_PROCESS_H
