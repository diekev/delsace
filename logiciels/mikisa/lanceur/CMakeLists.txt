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
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE mikisa)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
	mikisa_coeur
	mikisa_commandes
	mikisa_corps
	mikisa_entreface
	mikisa_operatrices
	mikisa_rendu
)

add_compile_options(-fPIC)

add_executable(${NOM_CIBLE} mikisa.cc)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

install(TARGETS ${NOM_CIBLE} RUNTIME DESTINATION ${NOM_PROJET})

################################################################################

set(NOM_CIBLE genere_menu_lcc)

add_executable(${NOM_CIBLE} genere_menu_lcc.cc)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} mikisa_lcc)

install(TARGETS ${NOM_CIBLE} RUNTIME DESTINATION ${NOM_PROJET})

################################################################################

set(NOM_CIBLE genere_fonctions_lcc_opengl)

add_executable(${NOM_CIBLE} genere_fonctions_lcc_opengl.cc)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} mikisa_lcc)

install(TARGETS ${NOM_CIBLE} RUNTIME DESTINATION ${NOM_PROJET})
