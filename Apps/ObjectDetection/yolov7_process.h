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

int Yolov7_Initialize(ObjectDetectionInfo* pInitData, std::string AppID);

std::vector<HailoDetectionPtr> Yolov7_Decode(ObjectDetectionInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov7_ShareDataCleanUp(ObjectDetectionData* pShareData);


void Yolov7_InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov7_ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov7_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);


#endif // YOLOV7_PROCESS_H
