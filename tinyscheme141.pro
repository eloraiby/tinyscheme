#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T21:30:12
#
#-------------------------------------------------

QT       -= core gui

TARGET = tinyscheme141
CONFIG   += console
CONFIG   -= app_bundle

#
# consider stripping the executable using: strip -s -R .comment -R .gnu.version -R .note -R .eh_frame -R .eh_frame_hdr tinyscheme141
#

QMAKE_CC	= gcc
QMAKE_CFLAGS	+= -std=c89 -fvisibility=hidden -pedantic -ffunction-sections -fdata-sections -DUSE_DL=1
QMAKE_LIBS	+= -ldl -lm
QMAKE_LFLAGS	+= -Wl,--gc-sections
QMAKE_LINK	= gcc


TEMPLATE = app

lexer.target = lexer.c
lexer.commands = ragel -C -o $$PWD/lexer.c $$PWD/lexer.rl
lexer.depends =

parser.target = parser.c
parser.commands = lemon -T$$PWD/lempar.c_template $$PWD/parser.y
parser.depends =

QMAKE_EXTRA_TARGETS	+= lexer parser

PRE_TARGETDEPS	+= lexer.c parser.c

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
    makefile \
    lempar.c_template \
    lexer.rl \
    parser.y

HEADERS += \
    CHANGES \
    dynload.h \
    opdefines.h \
    scheme.h \
    scheme-private.h
