#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T21:30:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = tinyscheme141
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CC = clang
QMAKE_CFLAGS += -std=c99
QMAKE_LIBS += -ldl

TEMPLATE = app


SOURCES += \
    dynload.c \
    scheme.c \
    number.c \
    error.c \
    frame.c \
    cell.c \
    gc.c \
    atom.c \
    stack.c \
    port.c \
    predicate.c

OTHER_FILES += \
    BUILDING \
    COPYING \
    hack.txt \
    init.scm \
    Manual.txt \
    MiniSCHEMETribute.txt

HEADERS += \
    CHANGES \
    dynload.h \
    opdefines.h \
    scheme.h \
    scheme-private.h \
    parser.h
