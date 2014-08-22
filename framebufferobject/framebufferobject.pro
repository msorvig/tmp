QT       += core gui widgets
OBJECTS_DIR = .obj
MOC_DIR = .moc
CONFIG -= app_bundle

TARGET = framebufferobject
TEMPLATE = app

SOURCES += main.cpp

SOURCES += \
    mainwidget.cpp \
    geometryengine.cpp \
    fbo.cpp \
    hellowindow.cpp \
    qtrenderer.cpp \
    quadrenderer.cpp

HEADERS += \
    mainwidget.h \
    geometryengine.h \
    fbo.h \
    hellowindow.h \
    qtrenderer.h \
    quadrenderer.h

RESOURCES += \
    shaders.qrc \
    textures.qrc

