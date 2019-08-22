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
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	bib_graphe
	bib_outils
	bib_patrons
	bib_vision
	bib_xml
	bib_structures
	danjo
	image
	memoire
	mikisa_corps
	pthread
	systeme_fichier
	wolika
	${BIBLIOTHEQUES_TBB}
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
	arbre_hbe.cc
	base_de_donnees.cc
	chef_execution.cc
	composite.cc
	configuration.cc
	contexte_evaluation.cc
	delegue_hbe.cc
	donnees_aval.cc
	donnees_simulation.cc
	entite.cc
	evenement.cc
	gestionnaire_fichier.cc
	image.cc
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
	tache.cc
	usine_operatrice.cc

	arbre_hbe.hh
	base_de_donnees.hh
	chef_execution.hh
	composite.h
	configuration.h
	contexte_evaluation.hh
	delegue_hbe.hh
	donnees_aval.hh
	donnees_simulation.hh
	entite.hh
	evenement.h
	gestionnaire_fichier.hh
	image.hh
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
	tache.h
	usine_operatrice.h
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_mikisa_coeur)
