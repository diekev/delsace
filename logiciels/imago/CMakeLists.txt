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
# The Original Code is Copyright (C) 2019 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_PROJET imago)

set(INCLUSIONS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/ui/
	${INCLUSIONS_QT}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT}
	ego
	stdc++fs
	bib_structures
	systeme_fichier
	bib_outils
)

qt5_wrap_cpp(
	MOC

	src/mainwindow.h
	src/user_preferences.h
)

qt5_wrap_ui(
	FORMS

	ui/mainwindow.ui
	ui/pref_window.ui
)

set(ENTETES
	src/mainwindow.h
    src/user_preferences.h
    src/glcanvas.h

	${FORMS}
)

set(SOURCES
	main.cc
	src/mainwindow.cc
    src/user_preferences.cc
    src/glcanvas.cc
)

set(OTHER_FILES
	src/gpu_shaders/frag.glsl
	src/gpu_shaders/vert.glsl
)

add_compile_options(-fPIC)

add_library(qt_imago ${MOC})

target_include_directories(qt_imago PUBLIC "${INCLUSIONS}")

desactive_drapeaux_compilation(qt_imago)

add_executable(${NOM_PROJET} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_PROJET} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}" qt_imago)

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${NOM_PROJET})
