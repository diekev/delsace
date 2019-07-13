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
# The Original Code is Copyright (C) 2017 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_BIBLIOTHEQUE systeme_fichier)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
	bib_structures
)

set(HEADERS
	file.h
	shared_library.h
	utilitaires.h
)

set(SOURCES
	file.cc
	shared_library.cc
	utilitaires.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_BIBLIOTHEQUE} "${BIBLIOTHEQUES}")

install(TARGETS ${NOM_BIBLIOTHEQUE} ARCHIVE DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/delsace/${NOM_BIBLIOTHEQUE})
