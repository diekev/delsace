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
# The Original Code is Copyright (C) 2020 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kuri_representation_intermediaire)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../compilation/
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${INCLUSIONS_LIBFFI}
)

set(BIBLIOTHEQUES
	kuri_arbre_syntaxique
	kuri_ipa_compilatrice
	kuri_compilation
	kuri_statistiques
	bib_outils
	systeme_fichier
	${BIBLIOTHEQUES_LIBFFI}
	dl
)

set(SOURCES
	analyse.cc
	code_binaire.cc
    constructrice_ri.cc
	impression.cc
	instructions.cc
	machine_virtuelle.cc
	optimisations.cc

	analyse.hh
	code_binaire.hh
    constructrice_ri.hh
	impression.hh
	instructions.hh
	machine_virtuelle.hh
	optimisations.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_dependencies(${NOM_CIBLE} kuri_arbre_syntaxique)
add_dependencies(${NOM_CIBLE} kuri_ipa_compilatrice)
