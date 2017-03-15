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
    src/OneDrive/msonedrive.cpp



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
    src/OneDrive/msonedrive.h


target.path = /usr/bin
INSTALLS += target

DISTFILES += \
    README.MD \
    doc/ccross.ronn

