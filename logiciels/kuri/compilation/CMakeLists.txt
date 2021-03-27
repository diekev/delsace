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
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

add_library(kuri_ipa_compilatrice ipa.cc ipa.hh)
target_include_directories(kuri_ipa_compilatrice PUBLIC "${INCLUSIONS}")
target_link_libraries(kuri_ipa_compilatrice kuri_parsage)
add_dependencies(kuri_ipa_compilatrice genere_ipa)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ipa.cc
	COMMAND genere_ipa "${CMAKE_CURRENT_SOURCE_DIR}/ipa.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/ipa.adn
	DEPENDS genere_ipa ${CMAKE_CURRENT_SOURCE_DIR}/../adn/ipa.adn
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ipa.hh
	COMMAND genere_ipa "${CMAKE_CURRENT_SOURCE_DIR}/ipa.hh" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/ipa.adn
	DEPENDS genere_ipa ${CMAKE_CURRENT_SOURCE_DIR}/../adn/ipa.adn
)

add_library(kuri_options_compilatrice options.cc options.hh)
target_include_directories(kuri_options_compilatrice PUBLIC "${INCLUSIONS}")
add_dependencies(kuri_options_compilatrice genere_options)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/options.cc
	COMMAND genere_options "${CMAKE_CURRENT_SOURCE_DIR}/options.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/options.adn
	DEPENDS genere_options ${CMAKE_CURRENT_SOURCE_DIR}/../adn/options.adn
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/options.hh
	COMMAND genere_options "${CMAKE_CURRENT_SOURCE_DIR}/options.hh" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/options.adn
	DEPENDS genere_options ${CMAKE_CURRENT_SOURCE_DIR}/../adn/options.adn
)

set(BIBLIOTHEQUES
	kuri_arbre_syntaxique
	kuri_ipa_compilatrice
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
	broyage.hh
	compilatrice.hh
	coulisse.hh
	coulisse_asm.hh
	coulisse_c.hh
	environnement.hh
	erreur.h
	espace_de_travail.hh
	graphe_dependance.hh
	message.hh
	metaprogramme.hh
	monomorphisations.hh
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

	broyage.cc
	compilatrice.cc
	coulisse.cc
	coulisse_asm.cc
	coulisse_c.cc
	environnement.cc
	erreur.cc
	espace_de_travail.cc
	graphe_dependance.cc
	message.cc
	metaprogramme.cc
	monomorphisations.cc
	operateurs.cc
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

add_dependencies(${NOM_CIBLE} kuri_ipa_compilatrice)
add_dependencies(${NOM_CIBLE} kuri_options_compilatrice)
