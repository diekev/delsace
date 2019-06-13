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

set(NOM_PROJET tests_unitaire_delsace)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
)

set(BIBLIOTHEQUES
	tests
	base64
	reseau
	sha256
	math
	types
)

set(SOURCES
	math/test_math.cc
	math/test_matrice.cc
	math/test_matrice_fixe.cc
	math/tests_matrice.cc
	math/tests_quaternion.cc
	math/tests_vecteur.cc

	tests.cc
	test_float.cc
	test_string.cc
	test_temperature.cc
	test_time.cc
	test_uri.cc

	math/tests_math.hh
	math/tests_matrice.hh
	math/tests_quaternion.hh
	math/tests_vecteur.hh

	test_uri.hh
	tests.hh
)

add_executable(${NOM_PROJET} ${SOURCES})

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

add_test(tests ${NOM_PROJET})

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION .)

set(NOM_PROJET tests_aleatoire_delsace)

set(SOURCES
	tests_aleatoire.cc
)

add_executable(${NOM_PROJET} ${SOURCES})

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")


add_test(tests ${NOM_PROJET})

install(TARGETS ${NOM_PROJET} RUNTIME DESTINATION .)
