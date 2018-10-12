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

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../coeur
    ${TEST_UNITAIRE_INCLUDE_DIRS}
)

set(BIBLIOTHEQUES
    ${TEST_UNITAIRE_LIBRARIES}
	decoupage
)

set(SOURCES
	outils.h
	test_decoupage.h
	test_decoupage_nombres.h
	test_expression.h
	test_fonctions.h
	test_structures.hh
	test_types.h
	test_unicode.h
	test_variables.h

	outils.cc
	test_decoupage.cc
	test_decoupage_nombres.cc
	test_expression.cc
	test_fonctions.cc
	test_structures.cc
	test_types.cc
	test_unicode.cc
	test_variables.cc
	tests.cc
)

add_executable(test_kuri ${SOURCES})

target_include_directories(test_kuri PUBLIC "${INCLUSIONS}")
target_link_libraries(test_kuri "${BIBLIOTHEQUES}")

install(
    FILES ${FICHIERS}
	DESTINATION exemples
)

add_test(tests test_kuri)

install(TARGETS test_kuri RUNTIME DESTINATION .)
