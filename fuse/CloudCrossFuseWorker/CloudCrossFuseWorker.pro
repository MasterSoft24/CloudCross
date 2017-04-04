TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    incominglistener.cpp \
    fswatcher.cpp
LIBS+= -lfuse \
        -pthread

HEADERS += \
    json.hpp \
    ccfw.h
