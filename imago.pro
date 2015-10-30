#-------------------------------------------------
#
# Project created by QtCreator 2015-02-11T20:26:09
#
#-------------------------------------------------

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = imago
TEMPLATE = app

CONFIG += c++11

QMAKE_CXXFLAGS += -O3 -msse -msse2 -msse3

QMAKE_CXXFLAGS_DEBUG += -g -Wall -Og -Wno-error=unused-function \
	-Wextra -Wno-missing-field-initializers -Wno-sign-compare -Wno-type-limits  \
	-Wno-unknown-pragmas -Wno-unused-parameter -Wno-ignored-qualifiers          \
	-Wmissing-format-attribute -Wno-delete-non-virtual-dtor                     \
	-Wsizeof-pointer-memaccess -Wformat=2 -Wno-format-nonliteral -Wno-format-y2k\
	-fstrict-overflow -Wstrict-overflow=2 -Wno-div-by-zero -Wwrite-strings      \
	-Wlogical-op -Wundef -DDEBUG_THREADS -Wnonnull -Wstrict-aliasing=2          \
	-fno-omit-frame-pointer -Wno-error=unused-result -Wno-error=clobbered       \
	-fstack-protector-all --param=ssp-buffer-size=4 -Wno-maybe-uninitialized    \
	-Wunused-macros -Wmissing-include-dirs -Wuninitialized -Winit-self          \
	-Wtype-limits -fno-common -fno-nonansi-builtins -Wformat-extra-args         \
	-Wno-error=unused-local-typedefs -DWARN_PEDANTIC -Winit-self -Wdate-time    \
	-Warray-bounds -Werror -fdiagnostics-color=always -fsanitize=address

QMAKE_LFLAGS += -fsanitize=address

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

INCLUDEPATH += src/ src/gpu/
INCLUDEPATH += /opt/lib/ego/include


LIBS += -lGLEW
LIBS += -L/opt/lib/ego/lib -lego
