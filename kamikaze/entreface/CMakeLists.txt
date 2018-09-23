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

set(NOM_CIBLE kamikaze_entreface)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../coeur/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_DANJO}
	${INCLUSIONS_EGO}
	${INCLUSIONS_QT5_OPENGL}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT5_WIDGETS}
	${BIBLIOTHEQUES_QT5_OPENGL}
	${BIBLIOTHEQUES_OPENGL}
	bib_opengl
	bib_texture
)

set(FICHIERS_DANJO
	scripts_danjo/disposition_ligne_temps.jo
	scripts_danjo/menu_debogage.jo
	scripts_danjo/menu_editeur_noeud.jo
	scripts_danjo/menu_edition.jo
	scripts_danjo/menu_fichier.jo
	scripts_danjo/menu_objet.jo
	scripts_danjo/menu_prereglage.jo
	scripts_danjo/operatrice_bruit.jo
	scripts_danjo/operatrice_commutateur.jo
	scripts_danjo/operatrice_couleur.jo
	scripts_danjo/operatrice_creation_boite.jo
	scripts_danjo/operatrice_creation_cercle.jo
	scripts_danjo/operatrice_creation_cone.jo
	scripts_danjo/operatrice_creation_courbes.jo
	scripts_danjo/operatrice_creation_grille.jo
	scripts_danjo/operatrice_creation_icosphere.jo
	scripts_danjo/operatrice_creation_nuage_point.jo
	scripts_danjo/operatrice_creation_torus.jo
	scripts_danjo/operatrice_creation_tube.jo
	scripts_danjo/operatrice_dispersion_points.jo
	scripts_danjo/operatrice_normal.jo
	scripts_danjo/operatrice_randomise_attribut.jo
	scripts_danjo/operatrice_suppression_attribut.jo
	scripts_danjo/operatrice_transformation.jo
	scripts_danjo/proprietes_objet.jo
)

set(NUANCEURS
	opengl/nuanceurs/flat_shader.frag
	opengl/nuanceurs/flat_shader.vert
	opengl/nuanceurs/object.frag
	opengl/nuanceurs/object.vert
	opengl/nuanceurs/tree_topology.frag
	opengl/nuanceurs/tree_topology.vert
	opengl/nuanceurs/volume.frag
	opengl/nuanceurs/volume.vert
)

add_compile_options(-fPIC)

qt5_wrap_cpp(
	test_hdr_moc

	editeur_arborescence.h
	editeur_canevas.h
	editeur_graphe.h
	editeur_ligne_temps.h
	editeur_proprietes.h
	fenetre_principale.h
)

add_library(${NOM_CIBLE} STATIC
	nodes/node_compound.h
	nodes/node_connection.h
	nodes/node_constants.h
	nodes/node_node.h
	nodes/node_port.h
	nodes/node_scene.h
	nodes/vue_editeur_noeud.h

	opengl/rendu_courbes.h
	opengl/rendu_maillage.h
	opengl/rendu_nuage_points.h
	opengl/rendu_primitive.h

	base_editeur.h
	editeur_arborescence.h
	editeur_canevas.h
	editeur_graphe.h
	editeur_ligne_temps.h
	editeur_proprietes.h
	fenetre_principale.h

	${test_hdr_moc}

	nodes/node_compound.cc
	nodes/node_connection.cc
	nodes/node_node.cc
	nodes/node_port.cc
	nodes/vue_editeur_noeud.cc

	opengl/rendu_courbes.cc
	opengl/rendu_maillage.cc
	opengl/rendu_nuage_points.cc
	opengl/rendu_primitive.cc

	base_editeur.cc
	editeur_arborescence.cc
	editeur_canevas.cc
	editeur_graphe.cc
	editeur_ligne_temps.cc
	editeur_proprietes.cc
	fenetre_principale.cc

	${FICHIERS_DANJO}
	${NUANCEURS}
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

install(
	FILES ${NUANCEURS}
	DESTINATION ${NOM_PROJET}/nuanceurs
)

install(
	FILES ${FICHIERS_DANJO}
	DESTINATION ${NOM_PROJET}/entreface
)
