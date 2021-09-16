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

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

################################################################################

set(NOM_BIBLIOTHEQUE kuri_outils_adn)

add_library(${NOM_BIBLIOTHEQUE} STATIC outils.cc outils.hh)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")

################################################################################

set(NOM_BIBLIOTHEQUE kuri_adn)

add_library(${NOM_BIBLIOTHEQUE} STATIC adn.cc adn.hh)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_BIBLIOTHEQUE} kuri_parsage kuri_outils_adn)
add_dependencies(${NOM_BIBLIOTHEQUE} kuri_parsage)

################################################################################

set(NOM_EXECUTABLE genere_lexemes)

set(BIBLIOTHEQUES
	kuri_outils_adn
	kuri_structures
  langage
)

add_executable(${NOM_EXECUTABLE} genere_lexemes.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_ipa)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

set(BIBLIOTHEQUES
	kuri_adn
	kuri_outils_adn
	kuri_parsage
	kuri_structures
	bib_structures
	bib_moultfilage
  langage
)

add_executable(${NOM_EXECUTABLE} genere_ipa.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

add_dependencies(${NOM_EXECUTABLE} kuri_parsage)

################################################################################

set(NOM_EXECUTABLE genere_options)

set(BIBLIOTHEQUES
	kuri_adn
	kuri_outils_adn
	kuri_parsage
	kuri_structures
	bib_structures
	bib_moultfilage
  langage
)

add_executable(${NOM_EXECUTABLE} genere_options.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_message)

set(BIBLIOTHEQUES
	kuri_adn
	kuri_outils_adn
	kuri_parsage
	kuri_structures
	bib_structures
	bib_moultfilage
  langage
)

add_executable(${NOM_EXECUTABLE} genere_message.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_noeuds)

set(BIBLIOTHEQUES
	kuri_adn
	kuri_outils_adn
	kuri_parsage
	kuri_structures
	bib_structures
	bib_moultfilage
  langage
)

add_executable(${NOM_EXECUTABLE} genere_noeuds.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")
