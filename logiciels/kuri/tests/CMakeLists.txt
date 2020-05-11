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
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_TESTS}
)

set(BIBLIOTHEQUES
	tests
	kuri_compilation
	kuri_coulisse_c
	kuri_coulisse_llvm
)

set(SOURCES
	test_decoupage.h

	test_decoupage.cc
	tests.cc
)

add_executable(test_kuri ${SOURCES})

target_include_directories(test_kuri PUBLIC "${INCLUSIONS}")
target_link_libraries(test_kuri "${BIBLIOTHEQUES}")

add_test(tests test_kuri)

install(TARGETS test_kuri RUNTIME DESTINATION ${RACINE_INSTALLATION})

add_executable(test_aleatoire_kuri test_donnees_aleatoire.cc)
target_include_directories(test_aleatoire_kuri PUBLIC "${INCLUSIONS}")
target_link_libraries(test_aleatoire_kuri "${BIBLIOTHEQUES}")

add_test(tests test_aleatoire_kuri)

install(TARGETS test_aleatoire_kuri RUNTIME DESTINATION ${RACINE_INSTALLATION})

add_executable(charge_fichier_aleatoire charge_fichier_aleatoire.cc)
target_include_directories(charge_fichier_aleatoire PUBLIC "${INCLUSIONS}")
target_link_libraries(charge_fichier_aleatoire "${BIBLIOTHEQUES}")

add_test(tests charge_fichier_aleatoire)

install(TARGETS charge_fichier_aleatoire RUNTIME DESTINATION ${RACINE_INSTALLATION})

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
