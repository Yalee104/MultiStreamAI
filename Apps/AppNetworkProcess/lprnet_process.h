#ifndef LPRNET_H
#define LPRNET_H

/* This file implements the lprnet pre and postprocess in cpp to improve the runtime
 * This is an intial implemntation further optimization are required.
*/
#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include "Utils/ctc_beam_search_decoder-master/src/ctc_beam_search_decoder.h"
#include "Apps/appcommon.h"

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QQueue>
#include <QPainter>
#include <QDebug>
#include <QThread>

//TODO: We should remove this dependencies due to tracker TAG is well embedded in yolov4Tiny_license_plate_det_process.h
//      Need to extract the dependencies to somewhere else.
#include "yolov4Tiny_license_plate_det_process.h"


constexpr int LPRNET_IMAGE_WIDTH = 300;
constexpr int LPRNET_IMAGE_HEIGHT = 75;


struct stLprResult
{
    std::string licensePlate = "";
    float confidence = 0.0f;
};


int LprNet_Initialize(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, std::string AppID);


void LprNet_InferWorker(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, 
                        AppImageData* pImageData);

stLprResult LprNet_ReadOutputWorker(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo);

#endif // LPRNET_H