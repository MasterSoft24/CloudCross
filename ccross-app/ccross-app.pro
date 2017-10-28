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

# for MSHttpRequest
include(../MSHttpRequest/MSHttpRequest.pri)
LIBS +=  -lcurl

QT += core network
QT -= gui

CONFIG += c++11

TARGET = ccross
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app


QMAKE_CXXFLAGS_WARN_OFF -= -Wno-unused-parameter

SOURCES += main.cpp \
    src/common/msrequest.cpp \
    src/common/msoptparser.cpp \
    src/common/mscloudprovider.cpp \
    src/GoogleDrive/msgoogledrive.cpp \
    src/common/msproviderspool.cpp \
    src/common/msremotefsobject.cpp \
    src/common/mslocalfsobject.cpp \
    src/common/msfsobject.cpp \
    src/common/msidslist.cpp \
    src/common/qstdout.cpp \
    src/Dropbox/msdropbox.cpp \
    src/YandexDisk/msyandexdisk.cpp \
    src/MailRu/msmailru.cpp \
    src/OneDrive/msonedrive.cpp \
    src/common/mssyncthread.cpp \
    ../MSHttpRequest/qmultibuffer.cpp \
    ../MSHttpRequest/mshttprequest.cpp \
    ../MSHttpRequest/QtCUrl.cpp \
    ../MSHttpRequest/msnetworkcookiejar.cpp \




HEADERS += \
    include/msrequest.h \
    include/msoptparser.h \
    include/mscloudprovider.h \
    src/GoogleDrive/msgoogledrive.h \
    include/msproviderspool.h \
    include/msremotefsobject.h \
    include/mslocalfsobject.h \
    include/msfsobject.h \
    include/msidslist.h \
    include/qstdout.h \
    src/Dropbox/msdropbox.h \
    src/YandexDisk/msyandexdisk.h \
    src/MailRu/msmailru.h \
    src/OneDrive/msonedrive.h \
    include/mssyncthread.h \
    ../MSHttpRequest/qmultibuffer.h \
    ../MSHttpRequest/mshttprequest.h \
    ../MSHttpRequest/QtCUrl.h \
    ../MSHttpRequest/msnetworkcookiejar.h \



target.path = /usr/bin
INSTALLS += target

DISTFILES += \
    README.MD \
    doc/ccross.ronn \
    ccross.pri \
    LICENSE \
    doc/ccross



# DEFINITIONS

DEFINES += GOOGLEDRIVE_CHUNK_SIZE=100*1024*1024 # max chunk size (minimum is a 262144 bytes)
DEFINES += ONEDRIVE_CHUNK_SIZE=10158080 #must be a multiplies of 32K
DEFINES += DROPBOX_CHUNK_SIZE=150*1024*1024


