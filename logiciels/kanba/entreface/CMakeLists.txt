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

set(NOM_CIBLE kanba_entreface)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${EGO_INCLUDE_DIRS}
	${INCLUSIONS_DANJO}
	${INCLUSIONS_SYSFICHIER}
        ${INCLUSIONS_QT_WIDGETS}
        ${INCLUSIONS_QT_OPENGL}
	${INCLUSIONS_DELSACE_MATH}
)

set(BIBLIOTHEQUES
        ${BIBLIOTHEQUES_QT_OPENGL}
	ego
	${BIBLIOTHEQUES_OPENGL}
	danjo
	math
	stdc++fs
	bib_opengl
        dls::outils
        kanba::coeur
)

add_compile_options(-fPIC)

set(ENTETES_POUR_QT
    base_editeur.h
    editeur_brosse.h
    editeur_calques.h
    editeur_canevas.h
    editeur_parametres.h
    fenetre_principale.h
    vue_region.hh
)

if(Qt6_FOUND)
    qt_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
else()
    qt5_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
endif()

add_library(qt_kanba_entreface "${moc_headers}")
target_include_directories(qt_kanba_entreface PUBLIC "${INCLUSIONS}")
target_link_libraries(qt_kanba_entreface PUBLIC ${BIBLIOTHEQUES} ${BIBLIOTHEQUES_QT_WIDGETS} ${BIBLIOTHEQUES_QT_OPENGL})

desactive_drapeaux_compilation(qt_kanba_entreface)

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
	scripts/brosse.jo
)

add_library(${NOM_CIBLE} STATIC
	opengl/rendu_brosse.h
	opengl/rendu_maillage.h
        opengl/rendu_seaux.hh
        opengl/tampons_rendu.hh
        opengl/textures.hh
	opengl/visionneur_image.h
	opengl/visionneur_scene.h

        ${ENTETES_POUR_QT}

        conversion_types.hh
        evenement_kanba.hh
        gestionnaire_interface.hh

	opengl/rendu_brosse.cc
	opengl/rendu_maillage.cc
        opengl/rendu_seaux.cc
        opengl/tampons_rendu.cc
        opengl/textures.cc
	opengl/visionneur_image.cc
	opengl/visionneur_scene.cc

	base_editeur.cc
        conversion_types.cc
	editeur_brosse.cc
	editeur_calques.cc
	editeur_canevas.cc
	editeur_parametres.cc
        evenement_kanba.cc
	fenetre_principale.cc
        gestionnaire_interface.cc
        vue_region.cc

	${NUANCEURS}
	${SCRIPTS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_kanba_entreface)

install(
	FILES ${SCRIPTS}
	DESTINATION ${NOM_PROJET}/scripts
)

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)
