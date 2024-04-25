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
# The Original Code is Copyright (C) 2020 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

find_package(OpenImageIO REQUIRED)
find_package(OpenEXR REQUIRED)

set(NOM_PROJET module_c_image)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${OPENIMAGEIO_INCLUDE_DIRS}
    ${OPENEXR_INCLUDE_DIRS}
)

set(BIBLIOTHEQUES
    ${OPENIMAGEIO_LIBRARIES}
    ${OPENEXR_LIBRARIES}

    dls::math
    dls::outils
)

set(SOURCES_ALGORITHMES
    analyse.cc
    application_fonction.cc
    champs_de_distance.cc
    filtrage.cc
    simulation_grain.cc

    analyse.hh
    application_fonction.hh
    champs_de_distance.hh
    donnees_canal.hh
    filtrage.hh
    simulation_grain.hh
)

add_library(bib_algorithmes_image STATIC "${SOURCES_ALGORITHMES}")
target_include_directories(bib_algorithmes_image PUBLIC "${INCLUSIONS}")
target_link_libraries(bib_algorithmes_image "${BIBLIOTHEQUES}")
set_property(TARGET bib_algorithmes_image PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(dls::algorithmes_image ALIAS bib_algorithmes_image)

set(SOURCES
    image.cc

    image.h
    oiio.h
)

list(APPEND BIBLIOTHEQUES dls::algorithmes_image)

compile_module_c(${NOM_PROJET} image image.h Image "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
set_property(TARGET ${NOM_PROJET}_statique PROPERTY CXX_STANDARD 14)
set_property(TARGET ${NOM_PROJET}_dynamique PROPERTY CXX_STANDARD 14)
