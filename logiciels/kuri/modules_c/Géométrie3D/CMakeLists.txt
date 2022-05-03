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

set(SOURCES
    creation.cc
    creation.h
    import_objet.cc
    import_objet.h
    ipa.cc
    ipa.hh
    outils.cc
    outils.hh

    ${CMAKE_CURRENT_SOURCE_DIR}/../InterfaceCKuri/contexte_kuri.hh
)

compile_module_c(${NOM_PROJET} geometrie3d ipa.hh Géométrie3D "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
