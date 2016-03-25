#-------------------------------------------------
#
# Project created by QtCreator 2015-06-07T16:43:04
#
#-------------------------------------------------

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FaceDetection
TEMPLATE = app

RC_FILE = resource.rc


SOURCES += main.cpp\
        mainwindow.cpp \
    detector.cpp

HEADERS  += mainwindow.h \
    detector.h

FORMS    += mainwindow.ui

INCLUDEPATH += C:\\OpenCV_2.4.111\\opencv\\build\\include

LIBS += $$quote(-LopencvDebug) opencv_core2411d.lib opencv_highgui2411d.lib opencv_imgproc2411d.lib opencv_objdetect2411d.lib
