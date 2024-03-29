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

set(NOM_CIBLE jorjala_entreface)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
        ${INCLUSIONS_JORJALA}
)

set(BIBLIOTHEQUES
	danjo
        ${BIBLIOTHEQUES_QT_WIDGETS}
        ${BIBLIOTHEQUES_QT_OPENGL}
        ${BIBLIOTHEQUES_OPENGL}
        ego
        bib_opengl
        dls::texture
        dls::outils
        dls::patrons_conception
        dls::vision
#	jorjala_coeur
#	jorjala_evaluation
        jorjala_rendu
)

add_compile_options(-fPIC)

set(ENTETES_POUR_QT
    barre_progres.hh
    base_editrice.h
#   editrice_arborescence.hh
    editrice_attributs.hh
    editrice_ligne_temps.h
    editrice_noeud.h
#   editrice_rendu.h
    editrice_vue2d.h
    editrice_vue3d.h
    fenetre_principale.h
    tache.h
    vue_region.hh
)

if(Qt6_FOUND)
    qt_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
else()
    qt5_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
endif()

add_library(qt_jorjala_entreface "${moc_headers}")
target_include_directories(qt_jorjala_entreface PUBLIC "${INCLUSIONS}")
target_link_libraries(qt_jorjala_entreface PRIVATE ${BIBLIOTHEQUES_QT_WIDGETS} ${BIBLIOTHEQUES_QT_OPENGL})

desactive_drapeaux_compilation(qt_jorjala_entreface)

set(NUANCEURS
	opengl/nuanceurs/couleur.frag
	opengl/nuanceurs/diffus.frag
	opengl/nuanceurs/diffus.vert
	opengl/nuanceurs/diffus_instances.vert
	opengl/nuanceurs/fonctions_normales.glsl
	opengl/nuanceurs/fonctions_polymorphiques.glsl
	opengl/nuanceurs/simple.frag
	opengl/nuanceurs/simple.vert
	opengl/nuanceurs/simple_instances.vert
	opengl/nuanceurs/texture.frag
	opengl/nuanceurs/volume.frag
	opengl/nuanceurs/volume.vert
)

set(FICHIERS_DANJO
	scripts_danjo/ajout_prise_noeud.jo
	scripts_danjo/ajouter_propriete.jo
	scripts_danjo/dialogue_creation_nuanceur.jo
	scripts_danjo/dialogue_renommage.jo
	scripts_danjo/disposition_ligne_temps.jo
	scripts_danjo/disposition_rendu.jo
	scripts_danjo/menu_ajouter_noeud_composite.jo
	scripts_danjo/menu_ajouter_noeud_detail.jo
	scripts_danjo/menu_ajouter_noeud_objet.jo
	scripts_danjo/menu_ajouter_noeud_rendu.jo
	scripts_danjo/menu_ajouter_noeud_simulation.jo
	scripts_danjo/menu_graphe_composite.jo
	scripts_danjo/menu_graphe_nuanceur.jo
	scripts_danjo/menu_graphe_objet.jo
	scripts_danjo/menu_graphe_rendu.jo
	scripts_danjo/menu_debogage.jo
	scripts_danjo/menu_edition.jo
	scripts_danjo/menu_fichier.jo
	scripts_danjo/menu_prereglage.jo
	scripts_danjo/objet.jo
	scripts_danjo/objet_camera.jo
	scripts_danjo/objet_lumiere.jo
	scripts_danjo/operatrice_3d_cercle.jo
	scripts_danjo/operatrice_3d_compression_normaux.jo
	scripts_danjo/operatrice_3d_cone.jo
	scripts_danjo/operatrice_3d_creation_attribut.jo
	scripts_danjo/operatrice_3d_creation_courbes.jo
	scripts_danjo/operatrice_3d_cube.jo
	scripts_danjo/operatrice_3d_cylindre.jo
	scripts_danjo/operatrice_3d_grille.jo
	scripts_danjo/operatrice_3d_ligne.jo
	scripts_danjo/operatrice_3d_lissage_laplacien.jo
	scripts_danjo/operatrice_3d_normaux.jo
	scripts_danjo/operatrice_3d_primitive_sphere.jo
	scripts_danjo/operatrice_3d_randomisation_attribut.jo
	scripts_danjo/operatrice_3d_sphere_ico.jo
	scripts_danjo/operatrice_3d_sphere_uv.jo
	scripts_danjo/operatrice_3d_suppression_attribut.jo
	scripts_danjo/operatrice_3d_torus.jo
	scripts_danjo/operatrice_advection_gaz.jo
	scripts_danjo/operatrice_affinage_gaz.jo
	scripts_danjo/operatrice_affinage_image.jo
	scripts_danjo/operatrice_ajout_corps_rigide.jo
	scripts_danjo/operatrice_analyse.jo
	scripts_danjo/operatrice_attr_couleur.jo
	scripts_danjo/operatrice_attribut_detail.jo
	scripts_danjo/operatrice_attribut_nuanceur.jo
	scripts_danjo/operatrice_attribut_promotion.jo
	scripts_danjo/operatrice_bilateral_image.jo
	scripts_danjo/operatrice_bruit_blanc.jo
	scripts_danjo/operatrice_bruit_cheveux.jo
	scripts_danjo/operatrice_champs_distance.jo
	scripts_danjo/operatrice_collision.jo
	scripts_danjo/operatrice_commutateur.jo
	scripts_danjo/operatrice_commutation_corps.jo
	scripts_danjo/operatrice_constante.jo
	scripts_danjo/operatrice_contraste.jo
	scripts_danjo/operatrice_coordonnees_polaires.jo
	scripts_danjo/operatrice_correction_couleur.jo
	scripts_danjo/operatrice_courbe_couleur.jo
	scripts_danjo/operatrice_creation_arbre.jo
	scripts_danjo/operatrice_creation_grille_eclairage.jo
	scripts_danjo/operatrice_creation_groupe.jo
	scripts_danjo/operatrice_creation_kelvinlet.jo
	scripts_danjo/operatrice_creation_pancartes.jo
	scripts_danjo/operatrice_creation_volume.jo
	scripts_danjo/operatrice_creation_volume_temporel.jo
	scripts_danjo/operatrice_daltonisme.jo
	scripts_danjo/operatrice_deformation.jo
	scripts_danjo/operatrice_deformation_kelvinlet.jo
	scripts_danjo/operatrice_degrade.jo
	scripts_danjo/operatrice_deep_scattering.jo
	scripts_danjo/operatrice_detail_camera.jo
	scripts_danjo/operatrice_detail_courbe_couleur.jo
	scripts_danjo/operatrice_detail_courbe_valeur.jo
	scripts_danjo/operatrice_detail_echantimage.jo
	scripts_danjo/operatrice_detail_rampe_couleur.jo
	scripts_danjo/operatrice_diffusion_gaz.jo
	scripts_danjo/operatrice_dilation_erosion.jo
	scripts_danjo/operatrice_dispersion_points.jo
	scripts_danjo/operatrice_dissipation_gaz.jo
	scripts_danjo/operatrice_enleve_doublons.jo
	scripts_danjo/operatrice_entree_gaz.jo
	scripts_danjo/operatrice_entree_simulation.jo
	scripts_danjo/operatrice_erosion_maillage.jo
	scripts_danjo/operatrice_evaluation_kelvinlet.jo
	scripts_danjo/operatrice_extraction_palette.jo
	scripts_danjo/operatrice_filtre.jo
	scripts_danjo/operatrice_flottance_gaz.jo
	scripts_danjo/operatrice_force_interaction.jo
	scripts_danjo/operatrice_flou.jo
	scripts_danjo/operatrice_fracture_voronoi.jo
	scripts_danjo/operatrice_fusion.jo
	scripts_danjo/operatrice_gamma.jo
	scripts_danjo/operatrice_geometrie_maillage.jo
	scripts_danjo/operatrice_generation_bruit_bleu.jo
	scripts_danjo/operatrice_gigue_points.jo
	scripts_danjo/operatrice_grade.jo
	scripts_danjo/operatrice_gravite.jo
	scripts_danjo/operatrice_import_alembic.jo
	scripts_danjo/operatrice_import_objet.jo
	scripts_danjo/operatrice_incrustation.jo
	scripts_danjo/operatrice_lecture_fichier.jo
	scripts_danjo/operatrice_lecture_objet.jo
	scripts_danjo/operatrice_lecture_video.jo
	scripts_danjo/operatrice_lueur_image.jo
	scripts_danjo/operatrice_maillage_vers_volume.jo
	scripts_danjo/operatrice_mappage_tonal_ondelette.jo
	scripts_danjo/operatrice_mediane_image.jo
	scripts_danjo/operatrice_melange.jo
	scripts_danjo/operatrice_melange_cheveux.jo
	scripts_danjo/operatrice_min_max.jo
	scripts_danjo/operatrice_normalisation.jo
	scripts_danjo/operatrice_normalise_covariance.jo
	scripts_danjo/operatrice_obstacle_gaz.jo
	scripts_danjo/operatrice_ocean.jo
	scripts_danjo/operatrice_ondelette_haar.jo
	scripts_danjo/operatrice_opensubdiv.jo
	scripts_danjo/operatrice_patch_triangle.jo
	scripts_danjo/operatrice_premultiplication.jo
	scripts_danjo/operatrice_projection_gaz.jo
	scripts_danjo/operatrice_points_depuis_image.jo
	scripts_danjo/operatrice_points_depuis_profonde.jo
	scripts_danjo/operatrice_poseidon_erosion.jo
	scripts_danjo/operatrice_rasterisation_prim.jo
	scripts_danjo/operatrice_rayons_soleil.jo
	scripts_danjo/operatrice_rendu_instance.jo
	scripts_danjo/operatrice_rendu_moteur.jo
	scripts_danjo/operatrice_script_detail.jo
	scripts_danjo/operatrice_simulation.jo
	scripts_danjo/operatrice_simulation_foule.jo
	scripts_danjo/operatrice_simulation_gaz.jo
	scripts_danjo/operatrice_simulation_grain.jo
	scripts_danjo/operatrice_simulation_vetement.jo
	scripts_danjo/operatrice_solveur_n_corps.jo
	scripts_danjo/operatrice_suppression_point.jo
	scripts_danjo/operatrice_terrain_erosion.jo
	scripts_danjo/operatrice_terrain_evaluation.jo
	scripts_danjo/operatrice_terrain_inclinaison.jo
	scripts_danjo/operatrice_terrain_vent.jo
	scripts_danjo/operatrice_texture.jo
	scripts_danjo/operatrice_tirage_fleche.jo
	scripts_danjo/operatrice_tonemap.jo
	scripts_danjo/operatrice_tournoiement.jo
	scripts_danjo/operatrice_trainee_points.jo
	scripts_danjo/operatrice_transfere_attribut.jo
	scripts_danjo/operatrice_transformation.jo
	scripts_danjo/operatrice_vent.jo
	scripts_danjo/operatrice_vis_arbre_hbe.jo
	scripts_danjo/operatrice_visibilite_camera.jo
	scripts_danjo/operatrice_visionnage.jo
	scripts_danjo/operatrice_visualisation_gaz.jo
	scripts_danjo/operatrice_vorticite_gaz.jo
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

        ${ENTETES_POUR_QT}
        chef_execution.hh
        dialogues.hh
        editrice_proprietes.h
        evenement_jorjala.hh
        gestion_entreface.hh
        gestionnaire_interface.hh

        graphe/item_noeud.cc
        graphe/vue_editrice_graphe.cc

        opengl/rendu_image.cc
        opengl/rendu_manipulatrice_2d.cc
        opengl/rendu_manipulatrice.cc
        opengl/visionneur_scene.cc

        barre_progres.cc
        base_editrice.cc
        chef_execution.cc
        dialogues.cc
#	editrice_arborescence.cc
        editrice_attributs.cc
        editrice_ligne_temps.cc
        editrice_noeud.cc
        editrice_proprietes.cc
#	editrice_rendu.cc
        editrice_vue2d.cc
        editrice_vue3d.cc
        evenement_jorjala.cc
	fenetre_principale.cc
        gestion_entreface.cc
        gestionnaire_interface.cc
        tache.cc
        vue_region.cc

	${FICHIERS_DANJO}
	${NUANCEURS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_jorjala_entreface)

install(
	FILES ${FICHIERS_DANJO}
	DESTINATION ${NOM_PROJET}/entreface
)

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)
