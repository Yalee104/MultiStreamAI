QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia concurrent

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Apps/AppNetworkProcess/arcface_process.cpp \
    Apps/AppNetworkProcess/yolov5_faceDet_process.cpp \
    Apps/AppNetworkProcess/yolov5_instance_seg_process.cpp \
    Apps/AppNetworkProcess/yolov5_process.cpp \
    Apps/AppNetworkProcess/yolov7_process.cpp \
    Apps/FaceRecognition/faceRecognition.cpp \
    Apps/ObjectDetection/objectdetection.cpp \
    Apps/Segmentation/segmentation.cpp \
    Apps/appbaseclass.cpp \
    Apps/appmanager.cpp \
    Utils/database/FaceDatabase.cpp \
    Utils/tracking/hailo_tracker.cpp \
    cameraview.cpp \
    main.cpp \
    mainwindow.cpp \
    mediastream.cpp \
    streamcontainer.cpp \
    streamview.cpp \
    videoview.cpp \

HEADERS += \
    Apps/AppNetworkProcess/arcface_process.h \
    Apps/AppNetworkProcess/yolov5_faceDet_process.h \
    Apps/AppNetworkProcess/yolov5_instance_seg_process.h \
    Apps/AppNetworkProcess/yolov5_process.h \
    Apps/AppNetworkProcess/yolov7_process.h \
    Apps/AppsFactory.h \
    Apps/FaceRecognition/faceRecognition.h \
    Apps/ObjectDetection/objectdetection.h \
    Apps/Segmentation/segmentation.h \
    Apps/appbaseclass.h \
    Apps/appcommon.h \
    Apps/appmanager.h \
    #Apps/appsequencer.h \
    Utils/database/FaceDatabase.hpp \
    Utils/hailo-common/hailo_common.hpp \
    Utils/hailo-common/hailo_objects.hpp \
    Utils/hailo-common/hailo_tensors.hpp \
    Utils/tracking/hailo_tracker.hpp \
    Utils/tracking/jde_tracker/jde_tracker.hpp \
    Utils/tracking/jde_tracker/jde_tracker_converters.hpp \
    Utils/tracking/jde_tracker/jde_tracker_embedding.hpp \
    Utils/tracking/jde_tracker/jde_tracker_ious.hpp \
    Utils/tracking/jde_tracker/jde_tracker_lapjv.hpp \
    Utils/tracking/jde_tracker/jde_tracker_strack_management.hpp \
    Utils/tracking/jde_tracker/jde_tracker_update.hpp \
    Utils/tracking/jde_tracker/kalman_filter.hpp \
    Utils/tracking/jde_tracker/lapjv.hpp \
    Utils/tracking/jde_tracker/strack.hpp \
    Utils/tracking/jde_tracker/tracker_macros.hpp \
    Utils/yolo-nms-decoder/yolo_nms_decoder.hpp \
    cameraview.h \
    mainwindow.h \
    mediastream.h \
    streamcontainer.h \
    streamview.h \
    videoview.h \

INCLUDEPATH += $$PWD/Utils/hailo-common
INCLUDEPATH += $$PWD/Utils/tracking
INCLUDEPATH += $$PWD/Utils/xtensor-master/include
INCLUDEPATH += $$PWD/Utils/xtl-master/include

FORMS += \
    mainwindow.ui

# Project Defines (Enable one only)
DEFINES += QT_ON_x86
# DEFINES += QT_ON_JETSON
# DEFINES += QT_ON_RK3588

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/jetsoon/Desktop
!isEmpty(target.path): INSTALLS += target

# Linux MultiNetworkPipeline
unix:!macx: LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline-Scheduler
DEPENDPATH += $$PWD/MultiNetworkPipeline-Scheduler
unix:!macx: PRE_TARGETDEPS += $$PWD/MultiNetworkPipeline-Scheduler/libMultiNetworkPipeline.a

# Windows MultiNetworkPipeline
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/release/ -lMultiNetworkPipeline
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/debug/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline-Scheduler
DEPENDPATH += $$PWD/MultiNetworkPipeline-Scheduler

# Linux HailoRT
unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lhailort
INCLUDEPATH += $$PWD/../../../../../usr/include/hailo
DEPENDPATH += $$PWD/../../../../../usr/include/hailo

# WINDOWS HailoRT
win32: LIBS += -L$$PWD/'../../../../Program Files/HailoRT/lib/' -llibhailort
INCLUDEPATH += $$PWD/'../../../../Program Files/HailoRT/include'
DEPENDPATH += $$PWD/'../../../../Program Files/HailoRT/include'

# Windows OpenCV
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/lib/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/lib/ -lopencv_world454d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/bin/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/bin/ -lopencv_world454d

INCLUDEPATH += $$PWD/../../../../OpenCV-4.5.4/opencv/build/include
DEPENDPATH += $$PWD/../../../../OpenCV-4.5.4/opencv/build/include

unix:CONFIG += link_pkgconfig
unix:PKGCONFIG += opencv4
