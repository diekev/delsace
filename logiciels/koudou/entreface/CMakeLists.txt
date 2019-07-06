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
# The Original Code is Copyright (C) 2017 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE koudou_entreface)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_QT5_OPENGL}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT5_WIDGETS}
	${BIBLIOTHEQUES_QT5_OPENGL}
	ego
	${BIBLIOTHEQUES_OPENGL}
	stdc++fs
	math
	bib_objets
	bib_opengl
	bib_texture
)

add_compile_options(-fPIC)

qt5_wrap_cpp(
	moc_headers

	controles/base_controles.h
	controles/controles.h

	base_editeur.h
	base_dialogue.h
	editeur_arbre_scene.h
	editeur_camera.h
	editeur_canevas.h
	editeur_objet.h
	editeur_material.h
	editeur_monde.h
	editeur_parametres.h
	editeur_rendu.h
	fenetre_principale.h
)

add_library(qt_koudou_entreface ${moc_headers})
target_include_directories(qt_koudou_entreface PUBLIC "${INCLUSIONS}")
get_target_property(EXTLIB_COMPILE_FLAGS qt_koudou_entreface COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
set_target_properties(qt_koudou_entreface PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

set(NUANCEURS
	opengl/nuanceurs/couleur.frag
	opengl/nuanceurs/diffus.frag
	opengl/nuanceurs/diffus.vert
	opengl/nuanceurs/simple.frag
	opengl/nuanceurs/simple.vert
	opengl/nuanceurs/texture.frag
)

add_library(${NOM_CIBLE} STATIC
	controles/assembleur_controles.h
	controles/base_controles.h
	controles/controles.h
	controles/usine_controles.h

	opengl/rendu_lumiere.h
	opengl/rendu_maillage.h
	opengl/rendu_monde.h
	opengl/visionneur_image.h
	opengl/visionneur_scene.h

	base_dialogue.h
	base_editeur.h
	editeur_arbre_scene.h
	editeur_camera.h
	editeur_canevas.h
	editeur_material.h
	editeur_monde.h
	editeur_parametres.h
	editeur_objet.h
	editeur_rendu.h
	fenetre_principale.h
	outils.h

	controles/assembleur_controles.cc
	controles/base_controles.cc
	controles/controles.cc
	controles/usine_controles.cc

	opengl/rendu_lumiere.cc
	opengl/rendu_maillage.cc
	opengl/rendu_monde.cc
	opengl/visionneur_image.cc
	opengl/visionneur_scene.cc

	base_dialogue.cc
	base_editeur.cc
	editeur_arbre_scene.cc
	editeur_camera.cc
	editeur_canevas.cc
	editeur_material.cc
	editeur_monde.cc
	editeur_parametres.cc
	editeur_objet.cc
	editeur_rendu.cc
	fenetre_principale.cc
	outils.cc

	${NUANCEURS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}" qt_koudou_entreface)

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)
