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

set(NOM_CIBLE poseidon_entreface)

set(INC_SYS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${EGO_INCLUDE_DIRS}
	${INCLUSIONS_DANJO}
	${INCLUSIONS_SYSFICHIER}
	${INCLUSIONS_QT5_WIDGETS}
	${INCLUSIONS_QT5_OPENGL}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT5_OPENGL}
	${BIBLIOTHEQUES_EGO}
	${BIBLIOTHEQUES_OPENGL}
	${BIBLIOTHEQUES_DANJO}
	stdc++fs
	bib_opengl
	bib_texture
)

add_compile_options(-fPIC)

qt5_wrap_cpp(
	moc_headers

	base_editeur.h
	editeur_canevas.h
	editeur_ligne_temps.h
	fenetre_principale.h
)

set(NUANCEURS
	opengl/nuanceurs/couleur.frag
	opengl/nuanceurs/diffus.frag
	opengl/nuanceurs/diffus.vert
	opengl/nuanceurs/simple.frag
	opengl/nuanceurs/simple.vert
	opengl/nuanceurs/texture.frag
	opengl/nuanceurs/texture_bombee.frag
	opengl/nuanceurs/texture_bombee.vert
)

set(SCRIPTS
	scripts/disposition_ligne_temps.jo
)

add_library(${NOM_CIBLE} STATIC
	opengl/rendu_champs_distance.h
	opengl/rendu_maillage.h
	opengl/rendu_particules.h
	opengl/visionneur_scene.h

	base_editeur.h
	editeur_canevas.h
	editeur_ligne_temps.h
	fenetre_principale.h

	${moc_headers}

	opengl/rendu_champs_distance.cc
	opengl/rendu_maillage.cc
	opengl/rendu_particules.cc
	opengl/visionneur_scene.cc

	base_editeur.cc
	editeur_canevas.cc
	editeur_ligne_temps.cc
	fenetre_principale.cc

	${NUANCEURS}
	${SCRIPTS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INC_SYS}")
target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

install(
	FILES ${SCRIPTS}
	DESTINATION ${NOM_PROJET}/scripts
)

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)
