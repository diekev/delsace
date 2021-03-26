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
# The Original Code is Copyright (C) 2021 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kuri_parsage)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
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
	base_syntaxeuse.cc
	gerante_chaine.cc
	identifiant.cc
	lexemes.cc
	lexeuse.cc
	modules.cc
	outils_lexemes.cc

	base_syntaxeuse.hh
	empreinte_parfaite.hh
	gerante_chaine.hh
	identifiant.hh
	lexemes.hh
	lexeuse.hh
	modules.hh
	outils_lexemes.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexemes.cc
	COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/lexemes.cc"
	DEPENDS genere_lexemes
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexemes.hh
	COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/lexemes.hh"
	DEPENDS genere_lexemes
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/empreinte_parfaite.hh
	COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/empreinte_parfaite.hh"
	DEPENDS genere_lexemes lexemes.hh
)

add_dependencies(${NOM_CIBLE} genere_lexemes)
