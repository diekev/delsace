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

set(NOM_BIBLIOTHEQUE booleens)

set(INCLUSIONS
	${INCLUSIONS_BOOST}
	${INCLUSIONS_CGAL}
)

set(SOURCES
	Boolean_Operations_Definitions.h
	Boolean_Operations_Items.h
	Boolean_Operations_Polyhedron.h
    Boolean_Operations_triangulation.h
    BoolPolyhedra.h
    CPolyhedron_from_polygon_builder_3.h
	mepp_config.h
	polyhedron.h
	polyhedron_enriched_polyhedron.h
	polyhedron_shared_items.h
	Polyhedron_Copy.h
	Polyhedron_OBJ_CGALImporter.h
	Polyhedron_OFF_CGALImporter.h
	Polyhedron_PLY_CGALImporter.h
	Polyhedron_SMF_CGALImporter.h

    dummy.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES})

target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC ${INCLUSIONS})
