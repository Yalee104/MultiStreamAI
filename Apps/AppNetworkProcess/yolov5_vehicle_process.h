#ifndef YOLOV5_VEHICLE_PROCESS_H
#define YOLOV5_VEHICLE_PROCESS_H

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

constexpr int YOLOV5_VEHICLE_FEATURE_MAP_SIZE1 = 20;
constexpr int YOLOV5_VEHICLE_FEATURE_MAP_SIZE2 = 40;
constexpr int YOLOV5_VEHICLE_FEATURE_MAP_SIZE3 = 80;
constexpr int YOLOV5_VEHICLE_TOTAL_CLASS = 1;
constexpr float YOLOV5_VEHICLE_CONFIDENCE_THRS = 0.75;

int Yolov5m_Vehicle_Initialize(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::string AppID);

std::vector<HailoDetectionPtr> Yolov5m_Vehicle_Decode(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov5m_Vehicle_ShareDataCleanUp(AppImageData* pImageData);

void Yolov5m_Vehicle_InferWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);

void Yolov5m_Vehicle_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);

void Yolov5m_Vehicle_VisualizeWorker(NetworkInferenceDetectionObjInfo* pVehicleObjInfo, AppImageData* pImageData);


#endif // YOLOV5_VEHICLE_PROCESS_H
