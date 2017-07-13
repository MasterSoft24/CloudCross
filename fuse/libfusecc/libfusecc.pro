#-------------------------------------------------
#
# Project created by QtCreator 2017-07-06T11:19:08
#
#-------------------------------------------------

android {
    message("* Using settings for Android.")

    DEFINES += PLATFORM_ANDROID
}


linux:!android {
    message("* Using settings for Unix/Linux.")

    DEFINES += PLATFORM_LINUX
}

win32{
    message("* Using settings for Windows.")

    DEFINES += PLATFORM_WINDOWS
}





QT       += network

QT       -= gui

TARGET = libfusecc
TEMPLATE = lib

DEFINES += LIBFUSECC_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


include(../../CloudCross.pri)

SOURCES += libfusecc.cpp \
    ccseparatethread.cpp

HEADERS += libfusecc.h\
        libfusecc_global.h \
    ccseparatethread.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
