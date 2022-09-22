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
# The Original Code is Copyright (C) 2018-2021 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kuri_arbre_syntaxique)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
	kuri_compilation
	kuri_parsage
	kuri_statistiques
	kuri_structures
	bib_outils
	bib_moultfilage
	bib_structures
	memoire
)

set(SOURCES
	allocatrice.cc
	assembleuse.cc
        copieuse.cc
	expression.cc
	infos_types.cc
	noeud_code.cc
	noeud_expression.cc
	utilitaires.cc

	allocatrice.hh
	assembleuse.hh
        copieuse.hh
	expression.hh
	infos_types.hh
	noeud_code.hh
	noeud_expression.hh
	utilitaires.hh
)

macro(genere_source __nom__)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.cc
		COMMAND genere_noeuds "${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/noeuds.adn
		DEPENDS genere_noeuds ../adn/noeuds.adn
	)

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.hh
		COMMAND genere_noeuds "${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.hh" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/noeuds.adn
		DEPENDS genere_noeuds ../adn/noeuds.adn
	)
endmacro()

genere_source(noeud_expression)
genere_source(noeud_code)
genere_source(allocatrice)
genere_source(assembleuse)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_dependencies(kuri_arbre_syntaxique genere_noeuds)
add_dependencies(${NOM_CIBLE} kuri_arbre_syntaxique)
