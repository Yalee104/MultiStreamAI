QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia concurrent

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Apps/ObjectDetection/objectdetection.cpp \
    Apps/appbaseclass.cpp \
    Apps/appmanager.cpp \
    Utils/tracking/hailo_tracker.cpp \
    cameraview.cpp \
    main.cpp \
    mainwindow.cpp \
    mediastream.cpp \
    streamcontainer.cpp \
    streamview.cpp \
    videoview.cpp \
    Apps/ObjectDetection/yolov5_process.cpp

HEADERS += \
    Apps/AppsFactory.h \
    Apps/ObjectDetection/objectdetection.h \
    Apps/appbaseclass.h \
    Apps/appmanager.h \
    Apps/appsequencer.h \
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
    Apps/ObjectDetection/yolov5_process.h

INCLUDEPATH += $$PWD/Utils/hailo-common
INCLUDEPATH += $$PWD/Utils/tracking
INCLUDEPATH += $$PWD/Utils/xtensor-master/include
INCLUDEPATH += $$PWD/Utils/xtl-master/include

FORMS += \
    mainwindow.ui

# Project Defines
# DEFINES += QT_ON_JETSON

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/jetsoon/Desktop
!isEmpty(target.path): INSTALLS += target

# Linux MultiNetworkPipeline
unix:!macx: LIBS += -L$$PWD/MultiNetworkPipeline/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline
DEPENDPATH += $$PWD/MultiNetworkPipeline
unix:!macx: PRE_TARGETDEPS += $$PWD/MultiNetworkPipeline/libMultiNetworkPipeline.a

# Windows MultiNetworkPipeline
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline/release/ -lMultiNetworkPipeline
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline/debug/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline
DEPENDPATH += $$PWD/MultiNetworkPipeline

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
