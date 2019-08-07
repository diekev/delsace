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
# The Original Code is Copyright (C) 2019 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE mikisa_poseidon)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
)

set(BIBLIOTHEQUES
	mikisa_coeur
	mikisa_corps
	wolika
	${BIBLIOTHEQUES_TBB}
)

add_library(${NOM_CIBLE} STATIC
	bruit_vaguelette.cc
	fluide.cc
	gradient_conjugue.cc
	incompressibilite.cc
	monde.cc
	particules.cc
	simulation.cc

	bruit_vaguelette.hh
	fluide.hh
	gradient_conjugue.hh
	incompressibilite.hh
	monde.hh
	particules.hh
	simulation.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

