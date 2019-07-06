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
)

set(BIBLIOTHEQUES
	ego
	${BIBLIOTHEQUES_TBB}
)

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

target_link_libraries(${NOM_CIBLE} PUBLIC ${BIBLIOTHEQUES})
