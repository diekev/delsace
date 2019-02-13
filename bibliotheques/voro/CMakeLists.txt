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

set(NOM_CIBLE bib_voro)

set(INCLUSIONS
)

add_library(${NOM_CIBLE} STATIC
    cell.cc
    c_loops.cc
    cmd_line.cc
    common.cc
    container.cc
    container_prd.cc
    pre_container.cc
    unitcell.cc
    v_base.cc
    v_compute.cc
    wall.cc

    cell.hh
    c_loops.hh
    common.hh
    config.hh
    container.hh
    container_prd.hh
    pre_container.hh
    rad_option.hh
    unitcell.hh
    v_base.hh
    v_compute.hh
    voro++.hh
    wall.hh
    worklist.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})
