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

set(NOM_CIBLE bib_opengl)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	/usr/include/freetype2/
)

set(BIBLIOTHEQUES
	ego
	${BIBLIOTHEQUES_TBB}
	bib_structures
	bib_outils
	freetype
)

set(FTGL
	texte/distance-field.c
	texte/edtaa3func.c
	texte/font-manager.c
	texte/text-buffer.c
	texte/texture-atlas.c
	texte/texture-font.c
	texte/utf8-utils.c
	texte/vector.c
	texte/vertex-attribute.c
	texte/vertex-buffer.c
	texte/distance-field.h
	texte/edtaa3func.h
	texte/font-manager.h
	texte/freetype-gl.h
	texte/markup.h
	texte/opengl.h
	texte/text-buffer.h
	texte/texture-atlas.h
	texte/texture-font.h
	texte/utf8-utils.h
	texte/vec234.h
	texte/vector.h
	texte/vertex-attribute.h
	texte/vertex-buffer.h
	texte/mat4.h
	texte/mat4.c
	texte/shader.h
	texte/shader.c
)

add_library(ftgl STATIC ${FTGL})
target_include_directories(ftgl PUBLIC "${INCLUSIONS}")
target_link_libraries(ftgl PUBLIC freetype)
get_target_property(EXTLIB_COMPILE_FLAGS ftgl COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wpedantic)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wold-style-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Woverloaded-virtual)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-delete-non-virtual-dtor)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wnon-virtual-dtor)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wdouble-promotion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=unused-variable)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=shadow)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=discarded-qualifiers)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -fconcepts)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -fno-nonansi-builtins)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=class-memaccess)
set_target_properties(ftgl PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")


add_library(${NOM_CIBLE} STATIC
    atlas_texture.cc
	contexte_rendu.cc
    pile_matrice.cc
	rendu_camera.cc
	rendu_grille.cc
	rendu_texte.cc
    tampon_rendu.cc

    atlas_texture.h
	contexte_rendu.h
    pile_matrice.h
	rendu_camera.h
	rendu_grille.h
	rendu_texte.h
    tampon_rendu.h
)

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})

target_link_libraries(${NOM_CIBLE} PUBLIC ${BIBLIOTHEQUES} ftgl)
