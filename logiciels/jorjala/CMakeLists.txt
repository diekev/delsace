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

set(NOM_PROJET jorjala)

add_subdirectory(coeur)

if(AVEC_CYCLES)
	add_subdirectory(cycles)

	#desactive_drapeaux_compilation(cycles)
	desactive_drapeaux_compilation(cycles_bvh)
	desactive_drapeaux_compilation(cycles_device)
	desactive_drapeaux_compilation(cycles_graph)
	desactive_drapeaux_compilation(cycles_kernel)
	desactive_drapeaux_compilation(cycles_render)
	desactive_drapeaux_compilation(cycles_subd)
	desactive_drapeaux_compilation(cycles_util)

	desactive_drapeaux_compilation_c(extern_clew)
	desactive_drapeaux_compilation_c(extern_cuew)
	desactive_drapeaux_compilation_c(extern_numaapi)
	desactive_drapeaux_compilation_c(extern_libc_compat)
endif()

add_subdirectory(commandes)
add_subdirectory(corps)
add_subdirectory(entreface)
add_subdirectory(evaluation)
add_subdirectory(koudou)
add_subdirectory(operatrices)
add_subdirectory(lanceur)
add_subdirectory(lcc)
add_subdirectory(poseidon)
add_subdirectory(rendu)
add_subdirectory(wolika)
