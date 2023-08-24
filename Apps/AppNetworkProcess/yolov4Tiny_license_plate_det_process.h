#ifndef LICENSE_PLATE_DET_PROCESS_H
#define LICENSE_PLATE_DET_PROCESS_H

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

#define LICENSE_PLATE_DET_TRACKER_USE

#ifdef LICENSE_PLATE_DET_TRACKER_USE

#include "hailo_tracker.hpp"

#endif


#define YOLOV4_TINY_LPD_TOTAL_CLASS         (1)
#define YOLOV4_TINY_LPD_CONFIDENCE_THRS     (0.5f)
#define YOLOV4_TINY_LPD_FEATURE_MAP_SIZE1   (13)
#define YOLOV4_TINY_LPD_FEATURE_MAP_SIZE2   (26)

int Yolov4TinyLicensePlate_Initialize(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, std::string AppID);

void Yolov4TinyLicensePlate_InferWorker(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, 
                                        AppImageData* pImageData);

bool Yolov4TinyLicensePlate_ReadOutputWorker(NetworkInferenceDetectionObjInfo* pLicensePlateDetInfo, std::vector<HailoDetectionPtr> &LpdDecodedResults);


#endif // LICENSE_PLATE_DET_PROCESS_H
