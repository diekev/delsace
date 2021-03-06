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

set(NOM_PROJET module_c_ftgl)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	/usr/include/freetype2/
)

set(BIBLIOTHEQUES
	freetype
)

add_library(${NOM_PROJET} STATIC
    distance-field.c
    edtaa3func.c
    mat4.c
    texture-atlas.c
    texture-font.c
    utf8-utils.c
    vector.c

	distance-field.h
	edtaa3func.h
	utf8-utils.h

    freetype-gl.h
    markup.h
    mat4.h
    opengl.h
    texture-atlas.h
    texture-font.h
    vec234.h
    vector.h
)

desactive_drapeaux_compilation_c(${NOM_PROJET})

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET}
	ARCHIVE
	DESTINATION /opt/bin/kuri/modules/FTGL/)
