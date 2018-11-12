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
	${INCLUSIONS_DELSACE_TESTS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUE_DELSACE_TESTS}
	decoupage
)

set(SOURCES
	outils.h
	test_assignation.hh
	test_boucle.hh
	test_decoupage.h
	test_decoupage_nombres.h
	test_expression.h
	test_fonctions.h
	test_modules.hh
	test_retour.hh
	test_structures.hh
	test_transtype.hh
	test_types.h
	test_unicode.h
	test_variables.h

	outils.cc
	test_assignation.cc
	test_boucle.cc
	test_decoupage.cc
	test_decoupage_nombres.cc
	test_expression.cc
	test_fonctions.cc
	test_modules.cc
	test_retour.cc
	test_structures.cc
	test_transtype.cc
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

add_executable(test_aleatoire_kuri test_donnees_aleatoire.cc)
target_include_directories(test_aleatoire_kuri PUBLIC "${INCLUSIONS}")
target_link_libraries(test_aleatoire_kuri "${BIBLIOTHEQUES}")

add_test(tests test_aleatoire_kuri)

install(TARGETS test_aleatoire_kuri RUNTIME DESTINATION .)

add_executable(charge_fichier_aleatoire charge_fichier_aleatoire.cc)
target_include_directories(charge_fichier_aleatoire PUBLIC "${INCLUSIONS}")
target_link_libraries(charge_fichier_aleatoire "${BIBLIOTHEQUES}")

add_test(tests charge_fichier_aleatoire)

install(TARGETS charge_fichier_aleatoire RUNTIME DESTINATION .)

set(FICHIERS_TESTS
	fichiers/module_test.kuri
	fichiers/test_fonction_inconnue_module.kuri
	fichiers/test_module_correcte.kuri
	fichiers/test_module_inconnu.kuri
	fichiers/test_utilisation_module_inconnu.kuri
)

install(
	FILES ${FICHIERS_TESTS}
	DESTINATION fichiers_tests
)
