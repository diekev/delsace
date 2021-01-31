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

set(NOM_PROJET himitsu)

set(INCLUSIONS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/forms/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_QT}
)

qt5_wrap_ui(
	FORMS

    forms/caesar.ui
    forms/enigma.ui
    forms/lorenz_sz40.ui
	forms/mainwindow.ui
    forms/vigenere.ui
)

qt5_wrap_cpp(
	MOC

	engines/cipherengine.h
	widgets/wheelwidget.h
	mainwindow.h
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT}
	bib_structures
)

set(ENTETES
    engines/cipherengine.h
	util/utils.h
	widgets/wheelwidget.h
	mainwindow.h

	${FORMS}
)

set(SOURCES
    engines/caesarcipher.cc
    engines/enigmacipher.cc
    engines/lorenzcipher.cc
    engines/vigenerecipher.cc
    util/utils.cc
    widgets/wheelwidget.cc
    main.cc
    mainwindow.cc
)

add_compile_options(-fPIC)

add_library(qt_himitsu ${MOC})

target_include_directories(qt_himitsu PUBLIC "${INCLUSIONS}")

desactive_drapeaux_compilation(qt_himitsu)

add_executable(${NOM_PROJET} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_PROJET} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}" qt_himitsu)

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${NOM_PROJET})
