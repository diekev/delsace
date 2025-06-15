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
# All rights réserved.
#
# ***** END GPL LICENSE BLOCK *****

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_TESTS}
)

set(BIBLIOTHEQUES kuri_compilation)

if(UNIX)
    set(SOURCES tests.cc)

    add_executable(test_kuri ${SOURCES})

    target_include_directories(test_kuri PUBLIC "${INCLUSIONS}")
    target_link_libraries(test_kuri "${BIBLIOTHEQUES}")

    add_test(NAME tests_unitaires COMMAND test_kuri)

    install(TARGETS test_kuri RUNTIME DESTINATION ${RACINE_INSTALLATION})
endif()

###############################################################

set(FICHIERS_TESTS
	fichiers/module_test.kuri
	fichiers/test_fonction_inconnue_module.kuri
	fichiers/test_module_correcte.kuri
	fichiers/test_module_inconnu.kuri
	fichiers/test_utilisation_module_inconnu.kuri
)

install(
	DIRECTORY fichiers
	DESTINATION ${RACINE_INSTALLATION}/fichiers_tests
)

################################################################################

add_executable(test_transformation test_transformation.cc)
target_include_directories(test_transformation PUBLIC "${INCLUSIONS}")
target_link_libraries(test_transformation "${BIBLIOTHEQUES}")

add_test(NAME test_transformation COMMAND test_transformation)
