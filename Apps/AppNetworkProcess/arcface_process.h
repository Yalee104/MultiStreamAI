#ifndef ARCFACE_PROCESS_H
#define ARCFACE_PROCESS_H

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include "Apps/appcommon.h"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQueue>
#include <QPainter>
#include <QDebug>
#include <QThread>

struct FaceRecognitionInfo {
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

void Arface_BuildFaceDB(const QString &FaceDBPath, FaceRecognitionInfo* pInitData);

int Arcface_Initialize(FaceRecognitionInfo* pFaceInfo, std::string AppID);

void ArcFace_InferWorker(FaceRecognitionInfo* pFaceInfo, ObjectDetectionInfo* pDetectionInfo, ObjectDetectionData* pDetectionData);

void ArcFace_ReadOutputWorker(FaceRecognitionInfo* pFaceInfo, ObjectDetectionData* pData);

void ArcFace_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData);

void Arcface_Test(const QString &TestImagePath, const QString &ImageFileName, FaceRecognitionInfo* pInitData);

#endif // ARCFACE_PROCESS_H
