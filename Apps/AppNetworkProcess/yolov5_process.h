#ifndef YOLOV5_PROCESS_H
#define YOLOV5_PROCESS_H

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

constexpr int FEATURE_MAP_SIZE1 = 20;
constexpr int FEATURE_MAP_SIZE2 = 40;
constexpr int FEATURE_MAP_SIZE3 = 80;
constexpr int TOTAL_CLASS = 80;
constexpr float CONFIDENCE_THRS = 0.5;

int Yolov5mInitialize(ObjectDetectionInfo* pInitData, std::string AppID);

std::vector<HailoDetectionPtr> Yolov5mDecode(ObjectDetectionInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov5mShareDataCleanUp(ObjectDetectionData* pShareData);


void InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);


#endif // YOLOV5_PROCESS_H
