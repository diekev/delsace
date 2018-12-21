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
# The Original Code is Copyright (C) 2018 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_CIBLE kanba_coeur)

set(INC_SYS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_SYSFICHIER}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_MATH}
	${BIBLIOTHEQUES_IMAGE}
	bib_observation
	bib_commandes
	bib_objets
	bib_vision
	bib_texture
	bib_transformation
)

add_compile_options(-fPIC)

add_library(${NOM_CIBLE} STATIC
	commandes/adaptrice_creation_maillage.h
	commandes/commandes_calques.h
	commandes/commandes_fichier.h
	commandes/commandes_vue2d.h
	commandes/commandes_vue3d.h

	brosse.h
	calques.h
	evenement.h
	kanba.h
	maillage.h
	melange.h
	outils_couleur.h
	paqueuse_texture.h

	commandes/adaptrice_creation_maillage.cc
	commandes/commandes_calques.cc
	commandes/commandes_fichier.cc
	commandes/commandes_vue2d.cc
	commandes/commandes_vue3d.cc

	brosse.cc
	calques.cc
	evenement.cc
	kanba.cc
	maillage.cc
	melange.cc
	outils_couleur.cc
	paqueuse_texture.cc
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INC_SYS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})
