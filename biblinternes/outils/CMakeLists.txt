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
# The Original Code is Copyright (C) 2017 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_BIBLIOTHEQUE bib_outils)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
	bib_structures
	langage
        ${BIBLIOTHEQUE_CXX_FILESYSTEM}
)

set(ENTETES
    assert.hh
	attendu.hh
    badge.hh
	chaine.hh
	chemin.hh
	constantes.h
	conditions.h
	definitions.h
	empreintes.hh
    enum_drapeau.hh
	fichier.hh
	format.hh
	gna.hh
	garde_portee.h
	indexeuse.hh
	iterateurs.h
	masque_binaire.h
	numerique.hh
	parametres.hh
    resultat.hh
    sauvegardeuse_etat.hh
	tableau_donnees.hh
	temps.hh
	transbit.hh
)

set(SOURCES
    assert.cc
	chaine.cc
	chemin.cc
	empreintes.cc
	fichier.cc
	format.cc
	gna.cc
	garde_portee.cc
	indexeuse.cc
	iterateurs.cc
	numerique.cc
	parametres.cc
    resultat.cc
	tableau_donnees.cc
	temps.cc
	main.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES})
add_library(dls::outils ALIAS bib_outils)
set_property(TARGET ${NOM_BIBLIOTHEQUE} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})
target_link_libraries(${NOM_BIBLIOTHEQUE} ${BIBLIOTHEQUES})
installe_dans_module_kuri(${NOM_BIBLIOTHEQUE} Kuri)
