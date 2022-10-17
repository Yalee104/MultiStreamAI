#-------------------------------------------------
#
# Project created by QtCreator 2022-09-27T14:45:18
#
#-------------------------------------------------

QT       -= core gui

TARGET = MultiNetworkPipeline
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MultiNetworkPipeline.cpp

HEADERS += \
    MultiNetworkPipeline.hpp
unix {
    target.path = /usr/lib
    INSTALLS += target
}

unix:!macx: LIBS += -L$$PWD/../../../../../../usr/lib/ -lhailort

INCLUDEPATH += $$PWD/../../../../../../usr/include/hailo
DEPENDPATH += $$PWD/../../../../../../usr/include/hailo

win32: LIBS += -L$$PWD/'../../../../../Program Files/HailoRT/lib/' -llibhailort

INCLUDEPATH += $$PWD/'../../../../../Program Files/HailoRT/include'
DEPENDPATH += $$PWD/'../../../../../Program Files/HailoRT/include'
