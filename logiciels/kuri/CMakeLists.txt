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
	add_definitions(-DAVEC_LLVM)
endif()

# ------------------------------------------------------------------------------

set(RACINE_INSTALLATION kuri)

add_subdirectory(compilation)
add_subdirectory(coulisse_c)
add_subdirectory(executable)
add_subdirectory(exemples)
add_subdirectory(tests)

if(AVEC_LLVM)
	add_subdirectory(coulisse_llvm)
endif()

set(DOSSIER_MODULES
	bibliotheques/Fondation
	bibliotheques/GlibC
	bibliotheques/Kuri
	bibliotheques/OpenSSL
	bibliotheques/PThread
	bibliotheques/Réseau
	bibliotheques/SysFichier
	bibliotheques/Tori
)

install(
	DIRECTORY ${DOSSIER_MODULES}
	DESTINATION ${RACINE_INSTALLATION}/modules
)
