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

set(NOM_CIBLE test_vulkan)
set(NOM_PROJET vulkan)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_MATH}
	#/home/kevin/src/lib/vulkan/x86_64/include/
	${INCLUSIONS_EGO}
)

set(BIBLIOTHEQUES
	glfw
	#/home/kevin/src/lib/vulkan/x86_64/lib/libvulkan.so
	GL
	GLEW
	${BIBLIOTHEQUES_EGO}
	bib_opengl
)

add_executable(${NOM_CIBLE}
	application.cc
	editrice.cc
	editrice_nulle.cc
	evenement.cc
	vulkan.cc

	application.hh
	editrice.hh
	editrice_nulle.hh
	evenement.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

install(TARGETS ${NOM_CIBLE} RUNTIME DESTINATION ${NOM_PROJET})
