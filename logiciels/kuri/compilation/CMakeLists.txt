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

set(NOM_CIBLE kuri_compilation)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
)

set(BIBLIOTHEQUES
	stdc++fs
	langage
	bib_outils
	bib_structures
	memoire
)

set(SOURCES
	arbre_syntactic.h
	assembleuse_arbre.h
	broyage.hh
	contexte_generation_code.h
	erreur.h
	expression.h
	graphe_dependance.hh
	lexemes.hh
	lexeuse.hh
	modules.hh
	operateurs.hh
	outils_lexemes.hh
	portee.hh
	structures.hh
	syntaxeuse.hh
	transformation_type.hh
	typage.hh
	validation_semantique.hh

	arbre_syntactic.cc
	assembleuse_arbre.cc
	broyage.cc
	contexte_generation_code.cc
	erreur.cc
	expression.cc
	graphe_dependance.cc
	lexemes.cc
	lexeuse.cc
	modules.cc
	operateurs.cc
	outils_lexemes.cc
	portee.cc
	structures.cc
	syntaxeuse.cc
	transformation_type.cc
	typage.cc
	validation_semantique.cc
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
