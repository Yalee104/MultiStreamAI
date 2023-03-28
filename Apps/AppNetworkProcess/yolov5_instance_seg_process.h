#ifndef YOLOV5_INS_SEG_PROCESS_H
#define YOLOV5_INS_SEG_PROCESS_H

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "Utils/yolo-nms-decoder/yolo_nms_decoder.hpp"
#include "Apps/appcommon.h"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQueue>
#include <QPainter>
#include <QDebug>
#include <QThread>

//Require OpenCV for better performance with Mask overlay, dissabling it will still work but will draw only bounding box and not mask.
#define USE_OPENCV

constexpr int YOLOV5_INSTANCE_SEG_MASK_MAP_SIZE = 160;      //Asuming H and W is the same, eg, 160x160
constexpr int YOLOV5_INSTANCE_SEG_MASK_FEATURE_SIZE = 32;
constexpr int YOLOV5_INS_SEG_FEATURE_MAP_SIZE1 = 20;
constexpr int YOLOV5_INS_SEG_FEATURE_MAP_SIZE2 = 40;
constexpr int YOLOV5_INS_SEG_FEATURE_MAP_SIZE3 = 80;
constexpr int YOLOV5_INS_SEG_TOTAL_CLASS = 80;
constexpr float YOLOV5_INS_SEG_CONFIDENCE_THRS = 0.5;

int Yolov5_Instance_Seg_Initialize(ObjectDetectionInfo* pInitData, std::string AppID);

std::vector<HailoDetectionPtr> Yolov5_Instance_Seg_Decode(ObjectDetectionInfo* pInitData, std::vector<std::vector<float32_t>> &OutputForDecode);

int Yolov5_Instance_Seg_ShareDataCleanUp(ObjectDetectionData* pShareData);


void Yolov5_Instance_Seg_InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov5_Instance_Seg_ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov5_Instance_Seg_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);


#endif // YOLOV5_INS_SEG_PROCESS_H
