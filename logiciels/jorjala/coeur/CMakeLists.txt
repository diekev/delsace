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

set(NOM_CIBLE jorjala_coeur)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	bib_outils
	bib_patrons
	bib_vision
	bib_xml
	bib_structures
	danjo
	image
	memoire
	jorjala_corps
	pthread
	systeme_fichier
	wolika
	${BIBLIOTHEQUES_TBB}
	stdc++fs
)

qt5_wrap_cpp(
	moc_headers

	tache.h
)

add_compile_options(-fPIC)

add_library(qt_jorjala_coeur ${moc_headers})
target_include_directories(qt_jorjala_coeur PUBLIC "${INCLUSIONS}")

desactive_drapeaux_compilation(qt_jorjala_coeur)

add_library(${NOM_CIBLE} STATIC
	arbre_hbe.cc
	base_de_donnees.cc
	chef_execution.cc
	compileuse_lcc.cc
	composite.cc
	configuration.cc
	contexte_evaluation.cc
	delegue_hbe.cc
	donnees_aval.cc
	donnees_simulation.cc
	editrice.cc
	evenement.cc
	gestionnaire_fichier.cc
	graphe.cc
	image.cc
	imprimeuse_graphe.cc
	jorjala.cc
	manipulatrice.cc
	noeud.cc
	noeud_image.cc
	nuanceur.cc
	objet.cc
	operatrice_corps.cc
	operatrice_graphe_detail.cc
	operatrice_image.cc
	operatrice_pixel.cc
	operatrice_simulation.cc
	outils_entreface.cc
	rendu.cc
	sauvegarde.cc
	tache.cc
	usine_operatrice.cc

	arbre_hbe.hh
	base_de_donnees.hh
	chef_execution.hh
	compileuse_lcc.hh
	composite.h
	configuration.h
	contexte_evaluation.hh
	delegue_hbe.hh
	donnees_aval.hh
	donnees_simulation.hh
	editrice.hh
	evenement.h
	gestionnaire_fichier.hh
	graphe.hh
	image.hh
	imprimeuse_graphe.h
	jorjala.hh
	manipulatrice.h
	noeud.hh
	noeud_image.h
	nuanceur.hh
	objet.h
	operatrice_corps.h
	operatrice_graphe_detail.hh
	operatrice_image.h
	operatrice_pixel.h
	operatrice_simulation.hh
	outils_entreface.hh
	rendu.hh
	sauvegarde.h
	tache.h
	usine_operatrice.h
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_jorjala_coeur)
