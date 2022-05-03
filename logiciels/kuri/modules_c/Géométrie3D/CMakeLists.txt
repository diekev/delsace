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
# The Original Code is Copyright (C) 2022 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_PROJET module_c_geometrie_3d)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
)

set(BIBLIOTHEQUES
)

set(SOURCES_VORONOI
    voro/cell.cc
    voro/c_loops.cc
    voro/cmd_line.cc
    voro/common.cc
    voro/container.cc
    voro/container_prd.cc
    voro/pre_container.cc
    voro/unitcell.cc
    voro/v_base.cc
    voro/v_compute.cc
    voro/wall.cc

    voro/cell.hh
    voro/c_loops.hh
    voro/common.hh
    voro/config.hh
    voro/container.hh
    voro/container_prd.hh
    voro/pre_container.hh
    voro/rad_option.hh
    voro/unitcell.hh
    voro/v_base.hh
    voro/v_compute.hh
    voro/voro++.hh
    voro/wall.hh
    voro/worklist.hh
)

set(SOURCES
    creation.cc
    creation.h
    import_objet.cc
    import_objet.h
    ipa.cc
    ipa.hh
    outils.cc
    outils.hh

    fracture.cc
    fracture.hh

    ${SOURCES_VORONOI}
    ${CMAKE_CURRENT_SOURCE_DIR}/../InterfaceCKuri/contexte_kuri.hh
)

compile_module_c(${NOM_PROJET} geometrie3d ipa.hh Géométrie3D "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
