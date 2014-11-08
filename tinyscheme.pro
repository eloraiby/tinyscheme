#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T21:30:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = tinyscheme
CONFIG   += console
CONFIG   -= app_bundle

#QMAKE_CC = clang
QMAKE_CFLAGS += -std=c99 -fvisibility=hidden -fvisibility-inlines-hidden -fvisibility-inlines-hidden -pedantic -ffunction-sections -fdata-sections
QMAKE_LIBS += -ldl
QMAKE_LFLAGS	+= -Wl,--gc-sections
INCLUDEPATH += ./include

TEMPLATE = app

lexer.target = $$PWD/langs/r4rs/lexer.c
lexer.commands = ragel -C -o $$PWD/langs/r4rs/lexer.c $$PWD/langs/r4rs/lexer.rl
lexer.depends =

parser.target = $$PWD/langs/r4rs/parser.c
parser.commands = lemon -T$$PWD/langs/lempar.c_template $$PWD/langs/r4rs/parser.yl
parser.depends =

QMAKE_EXTRA_TARGETS	+= lexer parser

PRE_TARGETDEPS	+= $$PWD/langs/r4rs/lexer.c $$PWD/langs/r4rs/parser.c

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
    src/predicate.c \
#    langs/r4rs/parser.c \
#    langs/r4rs/lexer.c \
    src/parse.c \
    src/eval.c \
    src/ioctl.c

OTHER_FILES += \
    CHANGES \
    docs/BUILDING \
    COPYING \
    docs/hack.txt \
    scm/init.scm \
    docs/Manual.txt \
    MiniSCHEMETribute.txt \
    scm/conform.scm \
    langs/lempar.c_template \
    langs/r4rs/lexer.rl \
    langs/r4rs/parser.yl


HEADERS += \
    include/dynload.h \
    src/opdefines.h \
    include/scheme.h \
    src/scheme-private.h \
    src/parser.h \
    langs/r4rs/parser.h
