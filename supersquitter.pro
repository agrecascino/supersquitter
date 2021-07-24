TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lSoapySDR -lnavajo -lpthread

QMAKE_CXXFLAGS = -g -O0

SOURCES += \
    main.cpp

HEADERS += \
    device.h \
    mod.h \
    server.h
