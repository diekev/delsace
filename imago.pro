#-------------------------------------------------
#
# Project created by QtCreator 2015-02-11T20:26:09
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = imago
TEMPLATE = app

CONFIG += c++11

QMAKE_CXXFLAGS_DEBUG += -fsanitize=address -fno-omit-frame-pointer

QMAKE_LFLAGS += -fsanitize=address

SOURCES += main.cpp\
		src/mainwindow.cpp \
	src/glwindow.cpp \
    src/linux_utils.cpp \
    user_preferences.cpp

HEADERS  += src/mainwindow.h \
	src/glwindow.h \
    src/linux_utils.h \
    user_preferences.h

FORMS    += ui/mainwindow.ui \
	ui/pref_window.ui

INCLUDEPATH += src/

RESOURCES += \
    resources.qrc
