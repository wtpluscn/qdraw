#-------------------------------------------------
#
# Project created by QtCreator 2015-06-13T23:45:28
#
#-------------------------------------------------

QT       += core gui xml svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include(../qtpropertybrowser/src/qtpropertybrowser.pri)
TARGET = qdraw
TEMPLATE = app

INCLUDEPATH += $$PWD/include

SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/drawobj.cpp \
    src/graphicsrectitem.cpp \
    src/graphicsellipseitem.cpp \
    src/graphicsarcitem.cpp \
    src/graphicstextitem.cpp \
    src/graphicsitemgroup.cpp \
    src/graphicspolygonitem.cpp \
    src/graphicslineitem.cpp \
    src/graphicsbezier.cpp \
    src/drawscene.cpp \
    src/drawtool.cpp \
    src/sizehandle.cpp \
    src/objectcontroller.cpp \
    src/customproperty.cpp \
    src/rulebar.cpp \
    src/drawview.cpp \
    src/texteditdialog.cpp \
    src/commands.cpp \
    src/document.cpp

HEADERS  += include/mainwindow.h \
    include/drawobj.h \
    include/graphicsrectitem.h \
    include/graphicsellipseitem.h \
    include/graphicsarcitem.h \
    include/graphicstextitem.h \
    include/graphicsitemgroup.h \
    include/graphicspolygonitem.h \
    include/graphicslineitem.h \
    include/graphicsbezier.h \
    include/drawshapes.h \
    include/drawscene.h \
    include/drawtool.h \
    include/sizehandle.h \
    include/objectcontroller.h \
    include/customproperty.h \
    include/rulebar.h \
    include/drawview.h \
    include/texteditdialog.h \
    include/commands.h \
    include/document.h

RESOURCES += \
    app.qrc

TRANSLATIONS = app_zh_CN.ts
