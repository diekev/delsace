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

TARGET = kyanbasu
TEMPLATE = app

QT += opengl

include(../global.pri)

SOURCES += \
	main.cc \
    fluid.cc \
    slab.cc \
    mainwindow.cc \
    glcanvas.cc

HEADERS += \
    fluid.h \
    slab.h \
    mainwindow.h \
    glcanvas.h

FORMS += \
    mainwindow.ui

OTHER_FILES += \
	gpu/shaders/advect.frag \
	gpu/shaders/background.frag \
	gpu/shaders/buoyancy.frag \
	gpu/shaders/divergence.frag \
	gpu/shaders/jacobi.frag \
	gpu/shaders/render.frag \
	gpu/shaders/simple.vert \
    gpu/shaders/splat.frag \
	gpu/shaders/subtract_gradient.frag

INCLUDEPATH += /opt/lib/numero7/include/numero7

LIBS += -L/opt/lib/numero7/lib/ -lego
LIBS += -lGLEW

unix {
	copy_files.commands = cp -r $$PWD/../kyanbasu/gpu/shaders/ .
}

QMAKE_EXTRA_TARGETS += copy_files
POST_TARGETDEPS += copy_files
