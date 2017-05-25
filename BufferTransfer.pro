#-------------------------------------------------
#
# Project created by QtCreator 2017-04-20T08:31:41
#
#-------------------------------------------------

QT += core network

QT -= gui

TARGET = BufferTransfer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = lib

SOURCES += main.cpp \
    transfer_server.cpp \
    transfer_config.cpp \
    transfer_client.cpp \
    library_exportfunction.cpp \
    shared_buffer.cpp \
    rtkcmn.c \
    rtcm3.c \
    rtcm.c

HEADERS += \
    transfer_server.h \
    transfer_config.h \
    transfer_client.h \
    common.h \
    shared_buffer.h \
    rtklib.h
