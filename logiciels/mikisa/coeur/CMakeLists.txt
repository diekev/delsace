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

add_subdirectory(operatrices/ariel)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_OPENCOLORIO}
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_WIDGETS}
	${INCLUSIONS_OPENEXR}
	${INCLUSIONS_OPENSUBDIV}
	${INCLUSIONS_BULLET}
	${INCLUSIONS_ALEMBIC}
	/usr/include/eigen3
)

set(BIBLIOTHEQUES
	danjo
	${BIBLIOTHEQUES_TBB}
	image
	bib_patrons
	bib_xml
	${BIBLIOTHEQUES_OPENCOLORIO}
	${BIBLIOTHEQUES_OPENEXR}
	bib_vision
	bib_objets
	bib_graphe
	bib_voro
	bib_structures
	${BIBLIOTHEQUES_OPENSUBDIV}
	fftw3
	pthread
	${BIBLIOTHEQUES_BULLET}
	bib_kelvinlet
	bib_outils
	memoire
	bib_corps
	bib_snh
	bib_lcc
	mikisa_ariel
	systeme_fichier
	${BIBLIOTHEQUES_ALEMBIC}
)

qt5_wrap_cpp(
	moc_headers

	tache.h
)

add_compile_options(-fPIC)

add_library(qt_mikisa_coeur ${moc_headers})
target_include_directories(qt_mikisa_coeur PUBLIC "${INCLUSIONS}")
get_target_property(EXTLIB_COMPILE_FLAGS qt_mikisa_coeur COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wconversion)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wsign-conversion)
set_target_properties(qt_mikisa_coeur PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

add_library(${NOM_CIBLE} STATIC
	commandes/commandes_edition.h
	commandes/commandes_noeuds.h
	commandes/commandes_objet.hh
	commandes/commandes_projet.h
	commandes/commandes_rendu.h
	commandes/commandes_temps.h
	commandes/commandes_vue2d.h
	commandes/commandes_vue3d.h

	evaluation/evaluation.hh
	evaluation/execution.hh
	evaluation/plan.hh
	evaluation/reseau.hh

	operatrices/arbre_bvh.hh
	operatrices/arbre_octernaire.hh
	operatrices/courbure.hh
	operatrices/limites_corps.hh
	operatrices/normaux.hh
	operatrices/ocean.hh
	operatrices/operatrices_3d.h
	operatrices/operatrices_alembic.hh
	operatrices/operatrices_arbre.hh
	operatrices/operatrices_ariel.hh
	operatrices/operatrices_attributs.hh
	operatrices/operatrices_bullet.hh
	operatrices/operatrices_cheveux.h
	operatrices/operatrices_corps.hh
	operatrices/operatrices_flux.h
	operatrices/operatrices_fracture.hh
	operatrices/operatrices_groupes.hh
	operatrices/operatrices_maillage.hh
	operatrices/operatrices_muscles.hh
	operatrices/operatrices_opensubdiv.hh
	operatrices/operatrices_particules.h
	operatrices/operatrices_pixel.h
	operatrices/operatrices_point3d.h
	operatrices/operatrices_poseidon.hh
	operatrices/operatrices_region.h
	operatrices/operatrices_script.hh
	operatrices/operatrices_simulation_foule.hh
	operatrices/operatrices_simulations.hh
	operatrices/operatrices_snh.hh
	operatrices/operatrices_vetements.hh
	operatrices/operatrices_visualisation.hh
	operatrices/operatrices_volume.hh

	base_de_donnees.hh
	chef_execution.hh
	composite.h
	configuration.h
	contexte_evaluation.hh
	donnees_aval.hh
	donnees_simulation.hh
	evenement.h
	gestionnaire_fichier.hh
	imprimeuse_graphe.h
	manipulatrice.h
	mikisa.h
	noeud_image.h
	objet.h
	operatrice_corps.h
	operatrice_graphe_maillage.h
	operatrice_graphe_pixel.h
	operatrice_image.h
	operatrice_pixel.h
	operatrice_simulation.hh
	sauvegarde.h
	scene.h
	tache.h
	usine_operatrice.h

	commandes/commandes_edition.cc
	commandes/commandes_noeuds.cc
	commandes/commandes_objet.cc
	commandes/commandes_projet.cc
	commandes/commandes_rendu.cc
	commandes/commandes_temps.cc
	commandes/commandes_vue2d.cc
	commandes/commandes_vue3d.cc

	evaluation/evaluation.cc
	evaluation/execution.cc
	evaluation/plan.cc
	evaluation/reseau.cc

	operatrices/arbre_bvh.cc
	operatrices/arbre_octernaire.cc
	operatrices/courbure.cc
	operatrices/limites_corps.cc
	operatrices/ocean.cc
	operatrices/normaux.cc
	operatrices/operatrices_3d.cc
	operatrices/operatrices_alembic.cc
	operatrices/operatrices_arbre.cc
	operatrices/operatrices_ariel.cc
	operatrices/operatrices_attributs.cc
	operatrices/operatrices_bullet.cc
	operatrices/operatrices_cheveux.cc
	operatrices/operatrices_corps.cc
	operatrices/operatrices_flux.cc
	operatrices/operatrices_fracture.cc
	operatrices/operatrices_groupes.cc
	operatrices/operatrices_maillage.cc
	operatrices/operatrices_muscles.cc
	operatrices/operatrices_opensubdiv.cc
	operatrices/operatrices_particules.cc
	operatrices/operatrices_pixel.cc
	operatrices/operatrices_point3d.cc
	operatrices/operatrices_poseidon.cc
	operatrices/operatrices_region.cc
	operatrices/operatrices_script.cc
	operatrices/operatrices_simulation_foule.cc
	operatrices/operatrices_simulations.cc
	operatrices/operatrices_snh.cc
	operatrices/operatrices_vetements.cc
	operatrices/operatrices_visualisation.cc
	operatrices/operatrices_volume.cc

	base_de_donnees.cc

	chef_execution.cc
	composite.cc
	configuration.cc
	contexte_evaluation.cc
	donnees_aval.cc
	donnees_simulation.cc
	evenement.cc
	gestionnaire_fichier.cc
	imprimeuse_graphe.cc
	manipulatrice.cc
	mikisa.cc
	noeud_image.cc
	objet.cc
	operatrice_corps.cc
	operatrice_graphe_maillage.cc
	operatrice_graphe_pixel.cc
	operatrice_image.cc
	operatrice_pixel.cc
	operatrice_simulation.cc
	sauvegarde.cc
	scene.cc
	tache.cc
	usine_operatrice.cc
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_mikisa_coeur)
