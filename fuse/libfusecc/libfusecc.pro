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


DEFINES +=CCROSS_LIB


QT       += network

QT       -= gui

TARGET = fusecc
VERSION = 1.0.0
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
    ccseparatethread.cpp \
    QtCUrl.cpp \
    ../../src/common/msproviderspool.cpp \
    ../../src/common/mscloudprovider.cpp \
    ../../src/common/msrequest.cpp \
    ../../src/common/qstdout.cpp \
    ../../src/common/msfsobject.cpp \
    ../../src/common/msremotefsobject.cpp \
    ../../src/common/mslocalfsobject.cpp \
    ../../src/OneDrive/msonedrive.cpp \
    ../../src/GoogleDrive/msgoogledrive.cpp \
    ../../src/Dropbox/msdropbox.cpp \
    ../../src/YandexDisk/msyandexdisk.cpp \
    ../../src/MailRu/msmailru.cpp \
    msnetworkcookiejar.cpp \
    ../../src/common/qmultibuffer.cpp \
    ../../src/common/mssyncthread.cpp

HEADERS += libfusecc.h\
        libfusecc_global.h \
    QtCUrl.h \
    ccseparatethread.h \
    ../../include/msproviderspool.h \
    ../../include/mscloudprovider.h \
    ../../include/msrequest.h \
    ../../include/qstdout.h \
    ../../include/msfsobject.h \
    ../../include/msremotefsobject.h \
    ../../include/mslocalfsobject.h \
    ../../src/OneDrive/msonedrive.h \
    ../../src/GoogleDrive/msgoogledrive.h \
    ../../src/Dropbox/msdropbox.h \
    ../../src/YandexDisk/msyandexdisk.h \
    ../../src/MailRu/msmailru.h \
    msnetworkcookiejar.h \
    ../../include/qmultibuffer.h \
    ../../include/mssyncthread.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}


LIBS +=  -lcurl


# DEFINITIONS

DEFINES += GOOGLEDRIVE_CHUNK_SIZE=100*1024*1024 # max chunk size (minimum is a 262144 bytes)
DEFINES += ONEDRIVE_CHUNK_SIZE=10158080 #must be a multiplies of 32K
DEFINES += DROPBOX_CHUNK_SIZE=150*1024*1024




