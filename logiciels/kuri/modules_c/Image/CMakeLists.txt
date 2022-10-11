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

    bib_outils
)

set(SOURCES
    image.cc

    image.h
    oiio.h
)

compile_module_c(${NOM_PROJET} image image.h Image "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")
set_property(TARGET ${NOM_PROJET}_statique PROPERTY CXX_STANDARD 14)
set_property(TARGET ${NOM_PROJET}_dynamique PROPERTY CXX_STANDARD 14)
