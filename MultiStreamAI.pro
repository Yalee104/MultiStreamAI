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
    cameraview.h \
    mainwindow.h \
    mediastream.h \
    streamcontainer.h \
    streamview.h \
    videoview.h \
    Apps/ObjectDetection/yolov5_process.h

FORMS += \
    mainwindow.ui

# Project Defines
DEFINES += QT_ON_JETSON

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/jetsoon/Desktop
!isEmpty(target.path): INSTALLS += target

unix:!macx: LIBS += -L$$PWD/MultiNetworkPipeline/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline
DEPENDPATH += $$PWD/MultiNetworkPipeline
unix:!macx: PRE_TARGETDEPS += $$PWD/MultiNetworkPipeline/libMultiNetworkPipeline.a

unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lhailort
INCLUDEPATH += $$PWD/../../../../../usr/include/hailo
DEPENDPATH += $$PWD/../../../../../usr/include/hailo
