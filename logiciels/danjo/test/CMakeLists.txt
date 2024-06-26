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
	${INCLUSIONS_SYSTEMES}
)

set(BIBLIOTHEQUES
	tests
	danjo
        dls::memoire
        dls::outils
)

add_executable(test_danjo
	test_decoupage.h
	test_initialisation.h

	test_decoupage.cc
	test_initialisation.cc
	tests.cc
)

target_include_directories(test_danjo PUBLIC "${INCLUSIONS}")

target_link_libraries(test_danjo ${BIBLIOTHEQUES})

install(
	TARGETS test_danjo RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

add_executable(tests_danjo_aleatoire
	tests_aleatoire.cc
)

target_include_directories(tests_danjo_aleatoire PUBLIC "${INCLUSIONS}")

target_link_libraries(tests_danjo_aleatoire ${BIBLIOTHEQUES})

install(
	TARGETS tests_danjo_aleatoire RUNTIME DESTINATION ${RACINE_INSTALLATION}
)
