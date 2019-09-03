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

set(NOM_PROJET mikisa_lcc)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
	mikisa_corps
	stdc++fs
	bib_phys
	memoire
	bib_structures
	langage
	bib_bruit
)

set(SOURCES
	analyseuse_grammaire.h
	arbre_syntactic.h
	assembleuse_arbre.h
	contexte_execution.hh
	contexte_generation_code.h
    decoupeuse.h
	donnees_type.h
	erreur.h
	execution_pile.hh
	expression.h
	fonctions.hh
	lcc.hh
	modules.hh
	morceaux.hh
	code_inst.hh

	analyseuse_grammaire.cc
	arbre_syntactic.cc
	assembleuse_arbre.cc
	contexte_execution.cc
	contexte_generation_code.cc
    decoupeuse.cc
	donnees_type.cc
	erreur.cc
	execution_pile.cc
	expression.cc
	fonctions.cc
	lcc.cc
	modules.cc
	morceaux.cc
	code_inst.cc
)

add_library(${NOM_PROJET} STATIC "${SOURCES}")

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

add_executable(test_lcc test_lcc.cc)

target_include_directories(test_lcc PUBLIC "${INCLUSIONS}")

target_link_libraries(test_lcc ${NOM_PROJET} memoire)
