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

set(NOM_CIBLE mikisa_koudou)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${INCLUSIONS_MATH}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_IMAGE}
	bib_outils
	bib_vision
)

add_library(${NOM_CIBLE} STATIC
	bsdf.cc
	koudou.cc
	lumiere.cc
	maillage.cc
	moteur_rendu.cc
	nuanceur.cc
	objet.cc
	pellicule.cc
	scene.cc
	statistiques.cc
	structure_acceleration.cc
	types.cc
	volume.cc

	bsdf.hh
	koudou.hh
	lumiere.hh
	maillage.hh
	objet.hh
	pellicule.hh
	moteur_rendu.hh
	nuanceur.hh
	scene.hh
	statistiques.hh
	structure_acceleration.hh
	types.hh
	volume.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
