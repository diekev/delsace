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

set(NOM_PROJET module_c_compression)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
)

add_library(${NOM_PROJET} STATIC
    compression.cc
    compression.h
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET}
	ARCHIVE
	DESTINATION /opt/bin/kuri/modules/Compression/)

add_dependencies(${NOM_PROJET} parcer)

set(FICHIER_MODULE "")

add_custom_command(
    OUTPUT /tmp/interface.kuri
    COMMAND parcer "${CMAKE_CURRENT_SOURCE_DIR}/compression.h" -o "/tmp/interface.kuri"
    DEPENDS parcer "${CMAKE_CURRENT_SOURCE_DIR}/compression.h"
    COMMENT "Compilation de ${CMAKE_CURRENT_SOURCE_DIR}/compression.h"
)
list(APPEND FICHIER_MODULE /tmp/interface.kuri)

add_custom_target(SOURCE_KURI_MODULE ALL DEPENDS "${FICHIER_MODULE}")

install(
    FILES "${FICHIER_MODULE}"
    DESTINATION /opt/bin/kuri/modules/Compression/)
