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

set(NOM_CIBLE bib_kelvinlet)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	/usr/include/eigen3/
	${INCLUSIONS_TBB}
)

set(BIBILIOTHEQUES
	${BIBILIOTHEQUES_EIGEN}
	${BIBILIOTHEQUES_TBB}
)

add_library(${NOM_CIBLE} STATIC
	brushAffine.cpp
	brushBase.cpp
	brushGrabBase.cpp
	dynaBase.cpp
	dynaPulseAffine.cpp
	dynaPulseBase.cpp
	dynaPulseGrab.cpp
	dynaPushAffine.cpp
	dynaPushBase.cpp
	dynaPushGrab.cpp

	brushAffine.h
	brushBase.h
	brushGrab.h
	brushGrabBase.h
	brushGrabBiLaplacian.h
	brushGrabBiScale.h
	brushGrabCusp.h
	brushGrabCuspBiLaplacian.h
	brushGrabCuspLaplacian.h
	brushGrabLaplacian.h
	brushGrabTriScale.h
	dynaBase.h
	dynaPulseAffine.h
	dynaPulseBase.h
	dynaPulseGrab.h
	dynaPushAffine.h
	dynaPushBase.h
	dynaPushGrab.h
	types.h

    kelvinlet.hh
)

desactive_drapeaux_compilation(${NOM_CIBLE})

target_include_directories(${NOM_CIBLE} PUBLIC ${INCLUSIONS})

target_link_libraries(${NOM_CIBLE} PUBLIC ${BIBLIOTHEQUES})
