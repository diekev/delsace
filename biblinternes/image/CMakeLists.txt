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

set(INCLUSIONS_SYSTEMES
	${JPEG_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(NOM_BIBLIOTHEQUE image)

set(ENTETES_LECTURE
	flux/ecriture.h
	flux/lecture.h
)

set(ENTETES_OPERATIONS
	operations/champs_distance.h
	operations/conversion.h
	operations/convolution.h
	operations/echantillonage.h
	operations/empreinte.h
	operations/melange.h
	operations/operations.h
)

set(ENTETES_OUTILS
	outils/couleurs.h
	outils/flux.h
)

set(ENTETES
	pixel.h
)

set(SOURCES
	flux/ecriture.cc
	flux/lecture.cc

	operations/conversion.cc
	operations/echantillonage.cc
	operations/empreinte.cc

	outils/flux.cc

	main.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES} ${ENTETES_OPERATIONS} ${ENTETES_LECTURE} ${ENTETES_OUTILS})

target_link_libraries(${NOM_BIBLIOTHEQUE} ${JPEG_LIBRARIES} stdc++fs bib_structures)

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${JPEG_INCLUDE_DIR})
