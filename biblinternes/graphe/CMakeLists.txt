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
# The Original Code is Copyright (C) 2018 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE bib_graphe)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
)

add_library(${NOM_CIBLE} STATIC
	compileuse_graphe.cc
	graphe.cc
	noeud.cc

	compileuse_graphe.h
	graphe.h
	noeud.h
)

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

################################################################################

set(NOM_BIBLIOTHEQUE graph_theory)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
	docopt
	pthread
	bib_structures
)

set(ENTETES
	dataflow.h
	layout.h
	graph.h
)

set(SOURCES
	main.cc
	matching.cc
	dataflow.cc
	layout.cc
	graph.cc
)

add_executable(${NOM_BIBLIOTHEQUE} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})

target_link_libraries(${NOM_BIBLIOTHEQUE} "${BIBLIOTHEQUES}")

install(TARGETS ${NOM_BIBLIOTHEQUE} RUNTIME DESTINATION ${NOM_BIBLIOTHEQUE})
