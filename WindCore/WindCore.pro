#-------------------------------------------------
#
# Project created by QtCreator 2019-12-18T16:18:09
#
#-------------------------------------------------

QT       -= core gui

TARGET = WindCore
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
    wind.cpp \
    isa-arm.c \
    decoder.c \
    decoder-arm.c \
    arm.c \
    emu.cpp

HEADERS += \
    wind_hw.h \
    wind.h \
    macros.h \
    isa-inlines.h \
    isa-arm.h \
    emitter-inlines.h \
    emitter-arm.h \
    decoder.h \
    decoder-inlines.h \
    common.h \
    arm.h \
    emu.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
