QT += core network
QT -= gui

CONFIG += c++11

TARGET = CloudCrossFuseDaemon
CONFIG += console
CONFIG -= app_bundle

include(../../CloudCross.pri)

TEMPLATE = app

SOURCES += main.cpp \
    commandserver.cpp \
    ccfdcommand.cpp \
    ../../src/common/msproviderspool.cpp \
    ../../src/common/mscloudprovider.cpp \
    ../../src/common/msrequest.cpp \
    ../../src/common/qstdout.cpp \
    ../../src/common/msfsobject.cpp \
    ../../src/common/msremotefsobject.cpp \
    ../../src/common/mslocalfsobject.cpp \
    ../../src/OneDrive/msonedrive.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    commandserver.h \
    ccfdcommand.h \
    ../../include/msproviderspool.h \
    ../../include/mscloudprovider.h \
    ../../include/msrequest.h \
    ../../include/qstdout.h \
    ../../include/msfsobject.h \
    ../../include/msremotefsobject.h \
    ../../include/mslocalfsobject.h \
    ../../src/OneDrive/msonedrive.h
