# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2015 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = imago
TEMPLATE = app

include(../../repos/seppuku/rcfiles/build_flags.pri)

QMAKE_CXXFLAGS -= -Weffc++

SOURCES += \
	main.cc                 \
	src/mainwindow.cc       \
    src/user_preferences.cc \
    src/glcanvas.cc

HEADERS += \
	src/mainwindow.h       \
    src/user_preferences.h \
    src/glcanvas.h

FORMS += \
	ui/mainwindow.ui \
	ui/pref_window.ui

OTHER_FILES += \
	src/gpu_shaders/frag.glsl \
	src/gpu_shaders/vert.glsl

unix {
	copy_files.commands = cp -r ../src/gpu_shaders/ .
}

QMAKE_EXTRA_TARGETS += copy_files
POST_TARGETDEPS += copy_files

INCLUDEPATH += src/
INCLUDEPATH += /opt/lib/ego/include

LIBS += -lGLEW
LIBS += /opt/lib/ego/lib/libego.a
