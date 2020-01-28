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
	analyseuse_grammaire.h
	arbre_syntactic.h
	assembleuse_arbre.h
	broyage.hh
	contexte_generation_code.h
	conversion_type_c.hh
	coulisse_c.hh
    decoupeuse.h
	donnees_type.h
	erreur.h
	expression.h
	generatrice_code_c.hh
	graphe_dependance.hh
	infos_type_c.hh
	modules.hh
	morceaux.hh
	operateurs.hh
	outils_morceaux.hh
	transformation_type.hh
	typeuse.hh
	validation_semantique.hh

	analyseuse_grammaire.cc
	arbre_syntactic.cc
	assembleuse_arbre.cc
	broyage.cc
	contexte_generation_code.cc
	conversion_type_c.cc
	coulisse_c.cc
    decoupeuse.cc
	donnees_type.cc
	erreur.cc
	expression.cc
	generatrice_code_c.cc
	graphe_dependance.cc
	infos_type_c.cc
	modules.cc
	morceaux.cc
	operateurs.cc
	outils_morceaux.cc
	transformation_type.cc
	typeuse.cc
	validation_semantique.cc
)

if(AVEC_LLVM)
	list(APPEND INCLUSIONS ${LLVM_INCLUDE_DIR})
	list(APPEND BIBLIOTHEQUES /usr/lib/llvm-6.0/lib/libLLVM.so) # À FAIRE
	list(APPEND SOURCES coulisse_llvm.cc coulisse_llvm.hh info_type.cc info_type.hh conversion_type_llvm.cc conversion_type_llvm.hh)
endif()

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
