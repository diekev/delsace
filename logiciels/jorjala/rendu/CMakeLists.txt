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

set(NOM_CIBLE jorjala_rendu)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../cycles/src/
	${CMAKE_CURRENT_SOURCE_DIR}/../cycles/third_party/atomic/
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${OPENIMAGEIO_INCLUDE_DIR}
)

add_definitions(
  -DCCL_NAMESPACE_BEGIN=namespace\ ccl\ {
  -DCCL_NAMESPACE_END=}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_OPENGL}
	ego
	bib_opengl
	bib_texture
	jorjala_coeur
        dls::outils
#	jorjala_koudou

#	cycles_render
#	cycles_util
#	cycles_bvh
#	cycles_device
#	cycles_graph
#	${OPENIMAGEIO_LIBRARY}
#	extern_numaapi
        ${BIBLIOTHEQUE_DL}
)

add_library(${NOM_CIBLE} STATIC
        extraction_données_corps.cc
	moteur_rendu.cc
        # moteur_rendu_cycles.cc
        # moteur_rendu_koudou.cc
	moteur_rendu_opengl.cc
        moteur_rendu_ratissage.cc
	rendu_corps.cc
	rendu_lumiere.cc

        extraction_données_corps.hh
        # cycles.hh
	moteur_rendu.hh
        # moteur_rendu_cycles.hh
        # moteur_rendu_koudou.hh
	moteur_rendu_opengl.hh
        moteur_rendu_ratissage.hh
	rendu_corps.h
	rendu_lumiere.h
)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})

