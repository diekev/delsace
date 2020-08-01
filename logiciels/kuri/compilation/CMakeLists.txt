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
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
	kuri_representation_intermediaire
	stdc++fs
	langage
	bib_outils
	bib_moultfilage
	bib_structures
	memoire
	iprof
)

set(SOURCES
	allocatrice_noeud.hh
	arbre_syntaxique.hh
	assembleuse_arbre.h
	broyage.hh
	compilatrice.hh
	environnement.hh
	erreur.h
	expression.h
	generation_code_c.hh
	graphe_dependance.hh
	identifiant.hh
	lexemes.hh
	lexeuse.hh
	message.hh
	modules.hh
	noeud_code.hh
	options.hh
	operateurs.hh
	outils_lexemes.hh
	portee.hh
	profilage.hh
	structures.hh
	syntaxeuse.hh
	tacheronne.hh
	transformation_type.hh
	typage.hh
	unite_compilation.hh
	validation_expression_appel.hh
	validation_semantique.hh

	allocatrice_noeud.cc
	arbre_syntaxique.cc
	assembleuse_arbre.cc
	broyage.cc
	compilatrice.cc
	environnement.cc
	erreur.cc
	expression.cc
	generation_code_c.cc
	graphe_dependance.cc
	identifiant.cc
	lexemes.cc
	lexeuse.cc
	message.cc
	modules.cc
	noeud_code.cc
	operateurs.cc
	options.cc
	outils_lexemes.cc
	portee.cc
	profilage.cc
	structures.cc
	syntaxeuse.cc
	tacheronne.cc
	transformation_type.cc
	typage.cc
	unite_compilation.cc
	validation_expression_appel.cc
	validation_semantique.cc
)

if(AVEC_LLVM)
	set(BIBLIOTHEQUES "${BIBLIOTHEQUES}" /usr/lib/llvm-6.0/lib/libLLVM.so) # À FAIRE
	set(SOURCES "${SOURCES}" generation_code_llvm.cc generation_code_llvm.hh)
endif()

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
