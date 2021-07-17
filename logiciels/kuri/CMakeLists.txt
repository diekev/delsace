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

# ------------------------------------------------------------------------------

option(AVEC_LLVM "Active l'utilisation de la bibliothèque LLVM" OFF)

if(AVEC_LLVM)
	find_package(LLVM REQUIRED)
	add_definitions(-DAVEC_LLVM)
endif()

# ------------------------------------------------------------------------------

set(RACINE_INSTALLATION kuri)

include(CTest)
enable_testing()

add_subdirectory(adn)
add_subdirectory(arbre_syntaxique)
add_subdirectory(compilation)
add_subdirectory(executable)
add_subdirectory(exemples)
add_subdirectory(modules_c/Alembic)
add_subdirectory(modules_c/Aléa)
add_subdirectory(modules_c/Compression)
add_subdirectory(modules_c/Exétron)
add_subdirectory(modules_c/FTGL)
add_subdirectory(modules_c/Image)
add_subdirectory(modules_c/Krypto)
add_subdirectory(parsage)
add_subdirectory(representation_intermediaire)
add_subdirectory(statistiques)
add_subdirectory(structures)
add_subdirectory(tests)

install(
	DIRECTORY modules
	DESTINATION ${RACINE_INSTALLATION}
)

set(DOSSIERS_TESTS
    exemples/applications/BDD
    exemples/applications/EditeurTexte
    exemples/applications/Eliza
    exemples/applications/Jeu
    exemples/applications/MoteurRendu
    exemples/applications/NASA
)

foreach(DOSSIER_TEST ${DOSSIERS_TESTS})
    get_filename_component(NOM_DOSSIER ${DOSSIER_TEST} NAME)
    add_test(NAME compilation_${NOM_DOSSIER} COMMAND kuri ${CMAKE_CURRENT_SOURCE_DIR}/${DOSSIER_TEST}/principale.kuri)
endforeach(DOSSIER_TEST)

# À FAIRE : exemples/applications/jeu_chaos.kuri
# À FAIRE : exemples/tests/test_tri.kuri
# À FAIRE : exemples/tests/test_fichier.kuri
set(FICHIERS_TESTS
    exemples/applications/base64.kuri
    exemples/applications/calendrier_républicain.kuri
    exemples/applications/cliente.kuri
    exemples/applications/client_smtp.kuri
    exemples/applications/crée_test_compilateur.kuri
    exemples/applications/json.kuri
    exemples/applications/labyrinthe.kuri
    exemples/applications/serveuse.kuri
    exemples/applications/serveuse_ssl.kuri
    exemples/applications/sudoku.kuri
    exemples/demos/demo_exécution.kuri
    exemples/demos/demo_expansion_argument_variadique.kuri
    exemples/demos/demo_info_type_fonction.kuri
    exemples/demos/demo_logement.kuri
    exemples/demos/demo_operateur_crochet.kuri
    exemples/demos/demo_operateur_point.kuri
    exemples/demos/demo_operateur_surcharge.kuri
    exemples/demos/demo_pointeurs.kuri
    exemples/demos/demo_reference.kuri
    exemples/demos/demo_si.kuri
    exemples/demos/demo_surcharge_fonctions.kuri
    exemples/demos/demo_syntax_appel_uniforme.kuri
    exemples/demos/demo_typage.kuri
    exemples/tests/test_assert_dans_fonction.kuri
    exemples/tests/test_annotations.kuri
    exemples/tests/test_boucle_pour.kuri
    exemples/tests/test_corps_texte.kuri
    exemples/tests/test_chaines_littérales.kuri
    exemples/tests/test_couleur_terminal.kuri
    exemples/tests/test_dessin_texte.kuri
    exemples/tests/test_emploi.kuri
    exemples/tests/test_énum.kuri
    exemples/tests/test_énum_drapeau.kuri
    exemples/tests/test_erreur.kuri
    exemples/tests/test_logement.kuri
    exemples/tests/test_memoire.kuri
    exemples/tests/test_metaprogrammation.kuri
    exemples/tests/test_metaprogramme_imprime.kuri
    exemples/tests/test_moultfilage.kuri
    exemples/tests/test_operateur.kuri
    exemples/tests/test_plage_boucle.kuri
    exemples/tests/test_polymorphisme.kuri
    exemples/tests/test_ppm.kuri
    exemples/tests/test_recherche.kuri
    exemples/tests/test_routage.kuri
    exemples/tests/test_structure_poly.kuri
    exemples/tests/test_sys_fichier.kuri
    exemples/tests/test_unicode.kuri
    modules/GHTML/test/test_ghtml.kuri
)

foreach(FICHIER_TEST ${FICHIERS_TESTS})
    get_filename_component(NOM_FICHIER ${FICHIER_TEST} NAME)
    add_test(NAME compilation_${NOM_FICHIER} COMMAND kuri ${CMAKE_CURRENT_SOURCE_DIR}/${FICHIER_TEST})
endforeach(FICHIER_TEST)

set(FICHIERS_TESTS_COROUTINE
    exemples/demos/demo_coro_nombre_premier.kuri
    exemples/demos/demo_coroutine.kuri
    exemples/demos/demo_retour_multiple.kuri
    exemples/tests/test_table_hachage.kuri
)

foreach(FICHIER_TEST ${FICHIERS_TESTS_COROUTINE})
    get_filename_component(NOM_FICHIER ${FICHIER_TEST} NAME)
    add_test(NAME compilation_${NOM_FICHIER} COMMAND kuri ${CMAKE_CURRENT_SOURCE_DIR}/${FICHIER_TEST})
    set_tests_properties(compilation_${NOM_FICHIER} PROPERTIES WILL_FAIL TRUE)
endforeach(FICHIER_TEST)

add_test(NAME test_spec_bdd COMMAND bundle exec rspec WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exemples/applications/BDD)
