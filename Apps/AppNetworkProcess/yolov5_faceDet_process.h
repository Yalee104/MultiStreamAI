#ifndef YOLOV5_FACE_DET_PROCESS_H
#define YOLOV5_FACE_DET_PROCESS_H

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

constexpr int YOLO_PERSON_FACE_FEATURE_MAP_SIZE1 = 20;
constexpr int YOLO_PERSON_FACE_FEATURE_MAP_SIZE2 = 40;
constexpr int YOLO_PERSON_FACE_FEATURE_MAP_SIZE3 = 80;
constexpr int YOLO_PERSON_FACE_TOTAL_CLASS = 2;
constexpr float YOLO_PERSON_FACE_CONFIDENCE_THRS = 0.5;

#define YOLO_PERSONFACE_PERSON_CLASS_ID         (1)
#define YOLO_PERSONFACE_FACE_CLASS_ID           (2)


int Yolov5_PersonFace_Initialize(ObjectDetectionInfo* pInitData, std::string AppID);

std::vector<HailoDetectionPtr> Yolov5PersonFaceDecode(ObjectDetectionInfo* pInitData, std::vector<std::vector<uint8_t>> &OutputForDecode);

int Yolov5_PersonFace_ShareDataCleanUp(ObjectDetectionData* pShareData);

void Yolov5_PersonFace_InferWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov5_PersonFace_ReadOutputWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Yolov5_PersonFace_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);


#endif // YOLOV5_FACE_DET_PROCESS_H
