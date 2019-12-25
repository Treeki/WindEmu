#-------------------------------------------------
#
# Project created by QtCreator 2019-12-18T16:17:36
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = WindQt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        pdascreenwindow.cpp

HEADERS += \
        mainwindow.h \
        pdascreenwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../WindCore/release/ -lWindCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../WindCore/debug/ -lWindCore
else:unix: LIBS += -L$$OUT_PWD/../WindCore/ -lWindCore

INCLUDEPATH += $$PWD/../WindCore
DEPENDPATH += $$PWD/../WindCore

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../WindCore/release/libWindCore.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../WindCore/debug/libWindCore.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../WindCore/release/WindCore.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../WindCore/debug/WindCore.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../WindCore/libWindCore.a
