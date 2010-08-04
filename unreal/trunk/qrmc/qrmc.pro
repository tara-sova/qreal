# #####################################################################
# Automatically generated by qmake (2.01a) Sun Jul 18 11:31:39 2010
# #####################################################################
TEMPLATE = app
QT += xml
TARGET =
DEPENDPATH += . \
	templates
INCLUDEPATH += . \
	../qrgui/kernel

CONFIG += rpath_libdirs
QMAKE_LFLAGS="-Wl,-O1,-rpath,$(PWD)"

# Input
HEADERS += metaCompiler.h \
	../qrgui/kernel/definitions.h \
	../qrrepo/repoApi.h \
	diagram.h \
	editor.h

SOURCES += main.cpp \
	metaCompiler.cpp \
	diagram.cpp \
	editor.cpp

LIBS += -L. \
	-lqrrepo # FIXME
OBJECTS_DIR = .obj
UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .moc

include (classes/classes.pri)
include (utils/utils.pri)
