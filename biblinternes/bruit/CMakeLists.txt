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

set(NOM_BIBLIOTHEQUE bib_bruit)

find_package(FFTW REQUIRED)

set(BIBLIOTHEQUES
    PUBLIC dls::outils
    PUBLIC dls::math
    PRIVATE FFTW::Double
)

set(SOURCES
    blanc.cc
    cellule.cc
    evaluation.cc
    flux.cc
    fourier.cc
    ondelette.cc
    outils.cc
    parametres.cc
    perlin.cc
    simplex.cc
    turbulent.cc
    voronoi.cc

    blanc.hh
    cellule.hh
    evaluation.hh
    flux.hh
    fourier.hh
    ondelette.hh
    outils.hh
    parametres.hh
    perlin.hh
    simplex.hh
    turbulent.hh
    voronoi.hh
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES})
add_library(dls::bruit ALIAS bib_bruit)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC .)
target_link_libraries(${NOM_BIBLIOTHEQUE} ${BIBLIOTHEQUES})
