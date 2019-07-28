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

set(NOM_PROJET kuri)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_CHRONO}
)

set(BIBLIOTHEQUES
	kuri_decoupage
	stdc++fs
	bib_outils
)

add_executable(kuri
    lanceur.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

install(FILES fichiers/execution_kuri.S DESTINATION ${RACINE_INSTALLATION}/fichiers)

################################################################################

add_executable(converti_html
	converti_html.cc
)

target_include_directories(converti_html PUBLIC "${INCLUSIONS}")

target_link_libraries(converti_html "${BIBLIOTHEQUES}")

install(
	TARGETS converti_html RUNTIME DESTINATION ${RACINE_INSTALLATION}
)
