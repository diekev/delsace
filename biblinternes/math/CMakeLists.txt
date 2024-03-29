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

set(NOM_BIBLIOTHEQUE math)

set(BIBLIOTHEQUES
    PUBLIC dls::outils
    PUBLIC dls::structures
)

set(ENTETES_MATRICE
    matrice/dimensions.hh
    matrice/matrice.hh
    matrice/matrice_fixe.hh
    matrice/operations.hh
)

set(ENTETES
    base_vecteur.hh
    boite_englobante.hh
    complexe.hh
    concepts.hh
    entrepolation.hh
    fourier.hh
    limites.hh
    matrice.hh
    outils.hh
    rmsd.hh
    rotation.hh
    statistique.hh
    swizzler.hh
    quaternion.hh
    rectangle.hh
    transformation.hh
    vecteur.hh
)

set(SOURCES
    boite_englobante.cc
    complexe.cc
    concepts.cc
    entrepolation.cc
    fourier.cc
    limites.cc
    matrice.cc
    outils.cc
    quaternion.cc
    rectangle.cc
    rmsd.cc
    rotation.cc
    transformation.cc
    vecteur.cc

    matrice/dimensions.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES} ${ENTETES_MATRICE})
set_property(TARGET ${NOM_BIBLIOTHEQUE} PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(dls::math ALIAS ${NOM_BIBLIOTHEQUE})
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC .)
target_link_libraries(${NOM_BIBLIOTHEQUE} ${BIBLIOTHEQUES})

################################################################################

set(NOM_BIBLIOTHEQUE test_mat)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
)

set(ENTETES
)

set(SOURCES
	test_mat.cc
)

add_executable(${NOM_BIBLIOTHEQUE} ${SOURCES} ${ENTETES})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})

target_link_libraries(${NOM_BIBLIOTHEQUE} ${BIBLIOTHEQUES})

