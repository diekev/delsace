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

set(NOM_CIBLE mikisa_ariel)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../
	${INCLUSIONS_BOOST}
	${INCLUSIONS_OPENEXR}
	${INCLUSIONS_OPENVDB}
)

set(BIBLIOTHEQUES
	math
	${BIBLIOTHEQUES_OPENVDB}
	bib_rmsd
	${BIBLIOTHEQUES_OPENEXR}
	z
	/opt/lib/blosc/lib/libblosc.so
	/opt/lib/boost/lib/libboost_iostreams.so
)

set(SOURCES
    geom/obj/obj.cpp
    geom/obj/obj.hpp
    geom/cubegen.cpp
    geom/cubegen.hpp
    geom/geom.cpp
    geom/geom.hpp
    geom/geomlist.hpp
    geom/mesh.cpp
    geom/mesh.hpp
    geom/meshgen.hpp
    geom/spheregen.cpp
    geom/spheregen.hpp
	grid/grid.hpp
    grid/levelset.cpp
    grid/levelset.hpp
    grid/particlegrid.cpp
    grid/particlegrid.hpp
    ray/ray.cpp
    ray/ray.hpp
    scene/scene.cpp
    scene/scene.hpp
    scene/sceneloader.cpp
    scene/sceneloader.hpp
    sim/flip.cpp
	sim/flip.hpp
    spatial/aabb.cpp
    spatial/aabb.hpp
    spatial/bvh.hpp
    spatial/spatial.cpp
    spatial/spatial.hpp
    utilities/datastructures.hpp
    utilities/loopvector.inl
    utilities/multiqueue.inl
    utilities/shortstack.inl
	utilities/utilities.cc
	utilities/utilities.h
)

add_library(${NOM_CIBLE} STATIC ${SOURCES})

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} ${BIBLIOTHEQUES})
