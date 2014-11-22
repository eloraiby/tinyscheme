#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T21:30:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = tinyscheme141
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CC	= gcc
QMAKE_CFLAGS	+= -std=c99 -fvisibility=hidden -fvisibility-inlines-hidden -pedantic -ffunction-sections -fdata-sections
QMAKE_LIBS	+= -ldl -lm
QMAKE_LFLAGS	+= -Wl,--gc-sections
QMAKE_LINK	= gcc

#QMAKE_CFLAGS += -std=c89
#QMAKE_LIBS += -ldl

TEMPLATE = app


SOURCES += \
    dynload.c \
    scheme.c \
    eval.c \
    atoms.c \
    ports.c \
    sexp.c

OTHER_FILES += \
    BUILDING \
    COPYING \
    hack.txt \
    init.scm \
    Manual.txt \
    MiniSCHEMETribute.txt \
    makefile

HEADERS += \
    CHANGES \
    dynload.h \
    opdefines.h \
    scheme.h \
    scheme-private.h
