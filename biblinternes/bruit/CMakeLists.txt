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

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(SOURCES
	blanc.cc
	cellule.cc
	evaluation.cc
	fourier.cc
	ondelette.cc
	parametres.cc
	perlin.cc
	simplex.cc
	turbulent.cc
	voronoi.cc

	blanc.hh
	cellule.hh
	evaluation.hh
	fourier.hh
	ondelette.hh
	parametres.hh
	perlin.hh
	simplex.hh
	turbulent.hh
	voronoi.hh
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})
