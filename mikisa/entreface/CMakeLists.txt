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
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE mikisa_entreface)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_DANJO}
	${INCLUSIONS_EGO}
	${INCLUSIONS_IMAGE}
	${INCLUSIONS_MATH}
	${INCLUSIONS_QT5_WIDGETS}
	${INCLUSIONS_QT5_OPENGL}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_DANJO}
	${BIBLIOTHEQUES_QT5_WIDGETS}
	${BIBLIOTHEQUES_QT5_OPENGL}
	${BIBLIOTHEQUES_OPENGL}
	${BIBLIOTHEQUES_EGO}
	bib_opengl
	bib_texture
)

add_compile_options(-fPIC)

qt5_wrap_cpp(
	moc_headers

	base_editrice.h
	editrice_ligne_temps.h
	editrice_noeud.h
	editrice_rendu.h
	editrice_vue2d.h
	editrice_vue3d.h
	fenetre_principale.h
)

set(NUANCEURS
	opengl/nuanceurs/couleur.frag
	opengl/nuanceurs/diffus.frag
	opengl/nuanceurs/diffus.vert
	opengl/nuanceurs/simple.frag
	opengl/nuanceurs/simple.vert
	opengl/nuanceurs/texture.frag
)

set(FICHIERS_DANJO
	scripts_danjo/ajouter_propriete.jo
	scripts_danjo/disposition_ligne_temps.jo
	scripts_danjo/disposition_rendu.jo
	scripts_danjo/menu_ajouter_noeud_composite.jo
	scripts_danjo/menu_ajouter_noeud_objet.jo
	scripts_danjo/menu_ajouter_noeud_point3d.jo
	scripts_danjo/menu_ajouter_noeud_scene.jo
	scripts_danjo/menu_debogage.jo
	scripts_danjo/menu_edition.jo
	scripts_danjo/menu_fichier.jo
	scripts_danjo/operatrice_3d_creation_attribut.jo
	scripts_danjo/operatrice_3d_creation_courbes.jo
	scripts_danjo/operatrice_3d_normaux.jo
	scripts_danjo/operatrice_3d_randomisation_attribut.jo
	scripts_danjo/operatrice_3d_suppression_attribut.jo
	scripts_danjo/operatrice_analyse.jo
	scripts_danjo/operatrice_camera.jo
	scripts_danjo/operatrice_champs_distance.jo
	scripts_danjo/operatrice_commutateur.jo
	scripts_danjo/operatrice_constante.jo
	scripts_danjo/operatrice_contraste.jo
	scripts_danjo/operatrice_coordonnees_polaires.jo
	scripts_danjo/operatrice_correction_couleur.jo
	scripts_danjo/operatrice_courbe_couleur.jo
	scripts_danjo/operatrice_daltonisme.jo
	scripts_danjo/operatrice_deformation.jo
	scripts_danjo/operatrice_degrade.jo
	scripts_danjo/operatrice_dilation_erosion.jo
	scripts_danjo/operatrice_filtre.jo
	scripts_danjo/operatrice_flou.jo
	scripts_danjo/operatrice_fusion.jo
	scripts_danjo/operatrice_gamma.jo
	scripts_danjo/operatrice_grade.jo
	scripts_danjo/operatrice_incrustation.jo
	scripts_danjo/operatrice_lecture_fichier.jo
	scripts_danjo/operatrice_lecture_objet.jo
	scripts_danjo/operatrice_melange.jo
	scripts_danjo/operatrice_min_max.jo
	scripts_danjo/operatrice_normalisation.jo
	scripts_danjo/operatrice_nuage.jo
	scripts_danjo/operatrice_objet.jo
	scripts_danjo/operatrice_ondelette_haar.jo
	scripts_danjo/operatrice_opensubdiv.jo
	scripts_danjo/operatrice_premultiplication.jo
	scripts_danjo/operatrice_point3d_math.jo
	scripts_danjo/operatrice_point3d_nuage.jo
	scripts_danjo/operatrice_point3d_traduction.jo
	scripts_danjo/operatrice_point3d_valeur.jo
	scripts_danjo/operatrice_point3d_vecteur.jo
	scripts_danjo/operatrice_saturation.jo
	scripts_danjo/operatrice_simulation_grain.jo
	scripts_danjo/operatrice_texture.jo
	scripts_danjo/operatrice_tonemap.jo
	scripts_danjo/operatrice_tournoiement.jo
	scripts_danjo/operatrice_traduction.jo
	scripts_danjo/operatrice_visionnage.jo
	scripts_danjo/parametres_projet.jo
	scripts_danjo/preferences.jo
)

add_library(${NOM_CIBLE} STATIC
	graphe/item_noeud.h
	graphe/vue_editrice_graphe.h

	opengl/rendu_image.h
	opengl/rendu_manipulatrice_2d.h
	opengl/rendu_manipulatrice.h
	opengl/visionneur_scene.h

	base_editrice.h
	editrice_ligne_temps.h
	editrice_noeud.h
	editrice_proprietes.h
	editrice_rendu.h
	editrice_vue2d.h
	editrice_vue3d.h
	fenetre_principale.h

	${moc_headers}

	graphe/item_noeud.cc
	graphe/vue_editrice_graphe.cc

	opengl/rendu_image.cc
	opengl/rendu_manipulatrice_2d.cc
	opengl/rendu_manipulatrice.cc
	opengl/visionneur_scene.cc

	base_editrice.cc
	editrice_ligne_temps.cc
	editrice_noeud.cc
	editrice_proprietes.cc
	editrice_rendu.cc
	editrice_vue2d.cc
	editrice_vue3d.cc
	fenetre_principale.cc

	${FICHIERS_DANJO}
	${NUANCEURS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

install(
	FILES ${FICHIERS_DANJO}
	DESTINATION ${NOM_PROJET}/entreface
)

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)
