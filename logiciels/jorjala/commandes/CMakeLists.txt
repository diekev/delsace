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
# The Original Code is Copyright (C) 2019 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE jorjala_commandes)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_OPENCOLORIO}
	${INCLUSIONS_OPENEXR}
	${INCLUSIONS_QT5_CORE}
)

# À FAIRE : module
set(BIBLIOTHEQUES_GRAPHVIZ
	cgraph
	gvc
)

set(BIBLIOTHEQUES
	bib_objets
	bib_outils
	bib_patrons
	bib_vision
	danjo
	jorjala_coeur
	jorjala_evaluation
	${BIBLIOTHEQUES_GRAPHVIZ}
	${BIBLIOTHEQUES_OPENCOLORIO}
	${BIBLIOTHEQUES_OPENEXR}
)

add_library(${NOM_CIBLE} STATIC
	commandes_edition.cc
	commandes_noeuds.cc
	commandes_objet.cc
	commandes_projet.cc
	commandes_rendu.cc
	commandes_temps.cc
	commandes_vue2d.cc
	commandes_vue3d.cc

	commandes_edition.h
	commandes_noeuds.h
	commandes_objet.hh
	commandes_projet.h
	commandes_rendu.h
	commandes_temps.h
	commandes_vue2d.h
	commandes_vue3d.h
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

