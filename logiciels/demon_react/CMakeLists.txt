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
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_CHRONO}
)

################################################################################

set(NOM_PROJET demon_react)

set(BIBLIOTHEQUES
        ${BIBLIOTHEQUE_CXX_FILESYSTEM}
	bib_outils
  systeme_fichier
  pthread
)

add_executable(${NOM_PROJET}
    demon.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${NOM_PROJET}
)

