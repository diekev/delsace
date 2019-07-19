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

set(NOM_PROJET hikage)

set(INCLUSIONS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUSIONS_QT}
)

qt5_wrap_ui(
	FORMS

	mainwindow.ui
    projectdialog.ui
)

qt5_wrap_cpp(
	MOC

	glcanvas.h
	mainwindow.h
	projectdialog.h
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT}
	bib_structures
	ego
)

set(ENTETES
	glcanvas.h
	mainwindow.h
    projectdialog.h
    project.h

	${FORMS}
)

set(SOURCES
	glcanvas.cc
	main.cc
	mainwindow.cc
    projectdialog.cc
    project.cc
)

add_compile_options(-fPIC)

add_library(qt_hikage ${MOC})

target_include_directories(qt_hikage PUBLIC "${INCLUSIONS}")
get_target_property(EXTLIB_COMPILE_FLAGS qt_hikage COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
set_target_properties(qt_hikage PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

add_executable(${NOM_PROJET} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_PROJET} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}" qt_hikage)

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${NOM_PROJET})
