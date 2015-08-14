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

SOURCES += \
	main.cc             \
	src/mainwindow.cc   \
	src/glwindow.cc     \
    src/linux_utils.cc  \
    src/user_preferences.cc

HEADERS += \
	src/mainwindow.h   \
	src/glwindow.h     \
    src/linux_utils.h  \
    user_preferences.h

FORMS    += ui/mainwindow.ui \
	ui/pref_window.ui

INCLUDEPATH += src/ \
	opt/lib/openexr/include/

RESOURCES += \
    resources.qrc
