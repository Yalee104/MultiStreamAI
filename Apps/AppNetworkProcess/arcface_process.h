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


void Arface_BuildFaceDB(const QString &FaceDBPath, NetworkInferenceBasedObjInfo* pInitData);

int Arcface_Initialize(NetworkInferenceBasedObjInfo* pFaceInfo, std::string AppID);

void ArcFace_InferWorker(NetworkInferenceBasedObjInfo* pFaceInfo, NetworkInferenceDetectionObjInfo* pDetectionInfo, AppImageData* pDetectionData);

void ArcFace_ReadOutputWorker(NetworkInferenceBasedObjInfo* pFaceInfo, NetworkInferenceDetectionObjInfo* pDetectionInfo, AppImageData* pData);

void ArcFace_VisualizeWorker(NetworkInferenceDetectionObjInfo* pInfo, AppImageData* pData);

void Arcface_Test(const QString &TestImagePath, const QString &ImageFileName, NetworkInferenceBasedObjInfo* pInitData);

#endif // ARCFACE_PROCESS_H
