#ifndef APPCOMMON_H
#define APPCOMMON_H

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <hailo/hailort.h>
#include "MultiNetworkPipeline-scheduler.hpp"
#include "Utils/hailo-common/hailo_common.hpp"

#include <QImage>

struct ObjectDetectionInfo {
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

struct ObjectDetectionData {

    std::vector<HailoDetectionPtr>      DecodedResult;
    QImage                              VisualizedImage;

};



#endif // APPCOMMON_H
