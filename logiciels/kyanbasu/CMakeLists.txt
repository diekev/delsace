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

set(NOM_PROJET kyanbasu)

set(INCLUSIONS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUSIONS_QT}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT}
	ego
	bib_structures
	bib_outils
)

qt5_wrap_ui(
	FORMS

	mainwindow.ui
)

qt5_wrap_cpp(
	MOC

	mainwindow.h
	glcanvas.h
)

set(ENTETES
    fluid.h
    slab.h
    mainwindow.h
    glcanvas.h

	${FORMS}
)

set(SOURCES
	main.cc
    fluid.cc
    slab.cc
    mainwindow.cc
    glcanvas.cc
)

set(OTHER_FILES
	gpu/shaders/advect.frag
	gpu/shaders/background.frag
	gpu/shaders/buoyancy.frag
	gpu/shaders/divergence.frag
	gpu/shaders/jacobi.frag
	gpu/shaders/render.frag
	gpu/shaders/simple.vert
    gpu/shaders/splat.frag
	gpu/shaders/subtract_gradient.frag
)

add_compile_options(-fPIC)

add_library(qt_kyanbasu ${MOC})

target_include_directories(qt_kyanbasu PUBLIC "${INCLUSIONS}")

desactive_drapeaux_compilation(qt_kyanbasu)

add_executable(${NOM_PROJET} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_PROJET} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}" qt_kyanbasu)

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${NOM_PROJET})
