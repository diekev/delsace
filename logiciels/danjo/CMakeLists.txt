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

set(INCLUSIONS_SYSTEMES
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_GUI}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT5_CORE}
	${BIBLIOTHEQUES_QT5_GUI}
	${BIBLIOTHEQUES_QT5_WIDGETS}
	bib_phys
	langage
	memoire
	bib_structures
	bib_outils

	stdc++fs
)

# ------------------------------------------------------------------------------

set(RACINE_INSTALLATION danjo)

add_subdirectory(coeur)
add_subdirectory(exemples)
add_subdirectory(test)
