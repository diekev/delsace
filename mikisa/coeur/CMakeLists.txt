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

set(NOM_CIBLE mikisa_coeur)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_DANJO}
	${INCLUSIONS_IMAGE}
	${INCLUSIONS_MATH}
	${INCLUSIONS_OPENCOLORIO}
	${INCLUSIONS_SYSFICHIER}
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_WIDGETS}
	${INCLUSIONS_OPENEXR}
	${INCLUSIONS_OPENSUBDIV}
	${INCLUSIONS_DELSACE_MATH}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_DANJO}
	${BIBLIOTHEQUES_MATH}
	${BIBLIOTHEQUES_SYSFICHIER}
	${BIBLIOTHEQUES_TBB}
	${BIBLIOTHEQUES_IMAGE}
	bib_audition
	bib_commandes
	bib_xml
	${BIBLIOTHEQUES_OPENCOLORIO}
	${BIBLIOTHEQUES_OPENEXR}
	bib_vision
	bib_objets
	bib_transformation
	bib_geometrie
	bib_graphe
	${BIBLIOTHEQUES_OPENSUBDIV}
)

qt5_wrap_cpp(
	moc_headers

	tache.h
)

add_compile_options(-fPIC)

add_library(${NOM_CIBLE} STATIC
	commandes/commandes_edition.h
	commandes/commandes_noeuds.h
	commandes/commandes_projet.h
	commandes/commandes_rendu.h
	commandes/commandes_temps.h
	commandes/commandes_vue2d.h
	commandes/commandes_vue3d.h

	corps/adaptrice_creation_maillage.h
	corps/corps.h
	corps/collection.h
	corps/courbes.h
	corps/groupes.h
	corps/listes.h
	corps/maillage.h
	corps/nuage_points.h

	operatrices/operatrices_3d.h
	operatrices/operatrices_cheveux.h
	operatrices/operatrices_flux.h
	operatrices/operatrices_particules.h
	operatrices/operatrices_pixel.h
	operatrices/operatrices_point3d.h
	operatrices/operatrices_region.h

	attribut.h
	composite.h
	configuration.h
	evaluation.h
	evenement.h
	imprimeuse_graphe.h
	manipulatrice.h
	mikisa.h
	noeud_image.h
	objet.h
	operatrice_corps.h
	operatrice_graphe_maillage.h
	operatrice_graphe_pixel.h
	operatrice_image.h
	operatrice_objet.h
	operatrice_pixel.h
	operatrice_scene.h
	sauvegarde.h
	scene.h
	tache.h
	usine_operatrice.h

	${moc_headers}

	commandes/commandes_edition.cc
	commandes/commandes_noeuds.cc
	commandes/commandes_projet.cc
	commandes/commandes_rendu.cc
	commandes/commandes_temps.cc
	commandes/commandes_vue2d.cc
	commandes/commandes_vue3d.cc

	corps/adaptrice_creation_maillage.cc
	corps/corps.cc
	corps/collection.cc
	corps/courbes.cc
	corps/groupes.cc
	corps/listes.cc
	corps/maillage.cc
	corps/nuage_points.cc

	operatrices/operatrices_3d.cc
	operatrices/operatrices_cheveux.cc
	operatrices/operatrices_flux.cc
	operatrices/operatrices_particules.cc
	operatrices/operatrices_pixel.cc
	operatrices/operatrices_point3d.cc
	operatrices/operatrices_region.cc

	attribut.cc
	composite.cc
	configuration.cc
	evaluation.cc
	evenement.cc
	imprimeuse_graphe.cc
	manipulatrice.cc
	mikisa.cc
	noeud_image.cc
	objet.cc
	operatrice_corps.cc
	operatrice_graphe_maillage.cc
	operatrice_graphe_pixel.cc
	operatrice_image.cc
	operatrice_objet.cc
	operatrice_pixel.cc
	operatrice_scene.cc
	sauvegarde.cc
	scene.cc
	tache.cc
	usine_operatrice.cc
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})
