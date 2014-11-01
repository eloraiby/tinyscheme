#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T21:30:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = tinyscheme
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CC = clang
QMAKE_CFLAGS += -std=c99
QMAKE_LIBS += -ldl
INCLUDEPATH += ./include

TEMPLATE = app


SOURCES += \
    src/dynload.c \
    src/scheme.c \
    src/number.c \
    src/error.c \
    src/frame.c \
    src/cell.c \
    src/gc.c \
    src/atom.c \
    src/stack.c \
    src/port.c \
    src/predicate.c

OTHER_FILES += \
    BUILDING \
    COPYING \
    hack.txt \
    init.scm \
    Manual.txt \
    MiniSCHEMETribute.txt

HEADERS += \
    CHANGES \
    include/dynload.h \
    src/opdefines.h \
    include/scheme.h \
    src/scheme-private.h \
    src/parser.h
