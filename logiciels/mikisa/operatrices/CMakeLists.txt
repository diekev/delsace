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

set(NOM_CIBLE mikisa_operatrices)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../danjo/coeur/
	${INCLUSIONS_ALEMBIC}
	${INCLUSIONS_BULLET}
	${INCLUSIONS_OPENEXR}
	${INCLUSIONS_OPENSUBDIV}
	/usr/include/eigen3
)

set(BIBLIOTHEQUES
	bib_bruit
	bib_kelvinlet
	bib_objets
	bib_outils
	bib_snh
	bib_structures
	bib_vision
	bib_voro
	danjo
	fftw3
	image
	pthread
	math
	memoire
	mikisa_corps
	mikisa_coeur
	mikisa_evaluation
	mikisa_lcc
	mikisa_poseidon
	opencv_core
	opencv_videoio
	png
	wolika
	${BIBLIOTHEQUES_ALEMBIC}
	${BIBLIOTHEQUES_BULLET}
	${BIBLIOTHEQUES_OPENSUBDIV}
	${BIBLIOTHEQUES_TBB}
)

add_library(${NOM_CIBLE} STATIC
	arbre_octernaire.cc
	courbure.cc
	outils_visualisation.cc
	normaux.cc
	operatrices_3d.cc
	operatrices_alembic.cc
	operatrices_arbre.cc
	operatrices_attributs.cc
	operatrices_bullet.cc
	operatrices_cheveux.cc
	operatrices_corps.cc
	operatrices_flux.cc
	operatrices_fracture.cc
	operatrices_groupes.cc
	operatrices_image_profonde.cc
	operatrices_images_3d.cc
	operatrices_maillage.cc
	operatrices_muscles.cc
	operatrices_ocean.cc
	operatrices_opensubdiv.cc
	operatrices_particules.cc
	operatrices_pixel.cc
	operatrices_poseidon.cc
	operatrices_region.cc
	operatrices_rendu.cc
	operatrices_script.cc
	operatrices_simulation_foule.cc
	operatrices_simulations.cc
	operatrices_snh.cc
	operatrices_srirp.cc
	operatrices_terrain.cc
	operatrices_uvs.cc
	operatrices_vetements.cc
	operatrices_visualisation.cc
	operatrices_volume.cc

	arbre_octernaire.hh
	courbure.hh
	normaux.hh
	outils_visualisation.hh
	operatrices_3d.h
	operatrices_alembic.hh
	operatrices_arbre.hh
	operatrices_attributs.hh
	operatrices_bullet.hh
	operatrices_cheveux.h
	operatrices_corps.hh
	operatrices_flux.h
	operatrices_fracture.hh
	operatrices_groupes.hh
	operatrices_image_profonde.hh
	operatrices_images_3d.hh
	operatrices_maillage.hh
	operatrices_muscles.hh
	operatrices_ocean.hh
	operatrices_opensubdiv.hh
	operatrices_particules.h
	operatrices_pixel.h
	operatrices_poseidon.hh
	operatrices_region.h
	operatrices_rendu.hh
	operatrices_script.hh
	operatrices_simulation_foule.hh
	operatrices_simulations.hh
	operatrices_snh.hh
	operatrices_srirp.hh
	operatrices_terrain.hh
	operatrices_uvs.hh
	operatrices_vetements.hh
	operatrices_visualisation.hh
	operatrices_volume.hh
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES} qt_mikisa_coeur)

