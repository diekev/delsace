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

set(NOM_CIBLE bib_structures)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
	memoire
)

add_library(${NOM_CIBLE} STATIC
	arbre_kd.cc
	chaine.cc
	chaine_utf8.cc
	dico.cc
	dico_desordonne.cc
	ensemble.cc
	file.cc
	flux_chaine.cc
	grille_particules.cc
	hachage_spatiale.cc
	liste.cc
	pile.cc
	tableau.cc
	vue_chaine.cc

	arbre_kd.hh
	chaine.hh
	chaine_utf8.hh
	dico.hh
	dico_desordonne.hh
	ensemble.hh
	file.hh
	flux_chaine.hh
	grille_particules.hh
	hachage_spatiale.hh
	liste.hh
	pile.hh
	tableau.hh
	vue_chaine.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
