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

# ------------------------------------------------------------------------------

option(AVEC_LLVM "Active l'utilisation de la bibliothèque LLVM" OFF)

if(AVEC_LLVM)
	find_package(LLVM REQUIRED)
	add_definitions(AVEC_LLVM)
endif()

# ------------------------------------------------------------------------------

add_subdirectory(coeur)
add_subdirectory(exemples)
add_subdirectory(tests)

set(FICHIERS_BIBLIOTHEQUES
	bibliotheques/adn.kuri
	bibliotheques/chaine.kuri
	bibliotheques/chrono.kuri
	bibliotheques/fichier.kuri
	bibliotheques/flux.kuri
	bibliotheques/info_type.kuri
	bibliotheques/iter.kuri
	bibliotheques/memoire.kuri
	bibliotheques/structures.kuri
	bibliotheques/uri.kuri
)
install(
	FILES ${FICHIERS_BIBLIOTHEQUES}
	DESTINATION bibliotheques
)
