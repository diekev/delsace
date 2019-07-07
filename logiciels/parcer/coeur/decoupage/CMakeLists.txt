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

set(NOM_CIBLE decoupage_parcer)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
)

message(${INCLUSIONS})

set(BIBLIOTHEQUES
	stdc++fs
	langage
	memoire
	bib_structures
)

set(SOURCES
    analyseuse_grammaire.hh
    arbre_syntactic.hh
    assembleuse_arbre.hh
    broyage.hh
    contexte_generation_code.hh
    decoupeuse.hh
    donnees_type.hh
    erreur.hh
    expression.hh
    modules.hh
    morceaux.hh
    nombres.hh
    validation_semantique.hh

    analyseuse_grammaire.cc
    arbre_syntactic.cc
    assembleuse_arbre.cc
    broyage.cc
    contexte_generation_code.cc
    decoupeuse.cc
    donnees_type.cc
    erreur.cc
    expression.cc
    modules.cc
    morceaux.cc
    nombres.cc
    validation_semantique.cc
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
