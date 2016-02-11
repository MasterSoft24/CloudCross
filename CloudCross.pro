QT += core network
QT -= gui

CONFIG += c++11

TARGET = ccross
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

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
    src/common/qstdout.cpp

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
    include/qstdout.h

target.path = /usr/bin
INSTALLS += target

DISTFILES += \
    README.MD \
    doc/ccross.1

