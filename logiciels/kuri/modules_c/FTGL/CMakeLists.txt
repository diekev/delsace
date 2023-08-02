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

find_package(Freetype REQUIRED)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${FREETYPE_INCLUDE_DIRS}
)

set(BIBLIOTHEQUES
	${FREETYPE_LIBRARY}
)

set(SOURCES
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
    ftgl.h
    markup.h
    mat4.h
    opengl.h
    texture-atlas.h
    texture-font.h
    vec234.h
    vector.h
)

compile_module_c(${NOM_PROJET} ftgl ftgl.h FTGL "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")

desactive_drapeaux_compilation_c(${NOM_PROJET}_statique)
desactive_drapeaux_compilation_c(${NOM_PROJET}_dynamique)
