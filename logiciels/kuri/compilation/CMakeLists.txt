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
	kuri_parsage
	kuri_representation_intermediaire
	kuri_statistiques
	kuri_structures
	stdc++fs
	langage
	bib_outils
	bib_moultfilage
	bib_structures
	memoire
)

set(SOURCES
	allocatrice_noeud.hh
	arbre_syntaxique.hh
	assembleuse_arbre.h
	broyage.hh
	compilatrice.hh
	coulisse.hh
	coulisse_asm.hh
	coulisse_c.hh
	environnement.hh
	erreur.h
	espace_de_travail.hh
	expression.h
	graphe_dependance.hh
	message.hh
	metaprogramme.hh
	monomorphisations.hh
	noeud_code.hh
	options.hh
	operateurs.hh
	portee.hh
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
	coulisse.cc
	coulisse_asm.cc
	coulisse_c.cc
	environnement.cc
	erreur.cc
	espace_de_travail.cc
	expression.cc
	graphe_dependance.cc
	message.cc
	metaprogramme.cc
	monomorphisations.cc
	noeud_code.cc
	operateurs.cc
	options.cc
	portee.cc
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
	set(SOURCES "${SOURCES}" coulisse_llvm.cc coulisse_llvm.hh)
endif()

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
