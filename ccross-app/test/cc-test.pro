
QT += core network
QT -= gui

CONFIG += c++11

include(../CloudCross.pri)

TARGET = cc-test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    msoptparser_test.cpp \
    ../src/common/msoptparser.cpp \
    ../src/common/qstdout.cpp

HEADERS += \
    msoptparser_test.h \
    ../include/qstdout.h \
    ../include/msoptparser.h
