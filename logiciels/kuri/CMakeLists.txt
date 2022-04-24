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

# À FAIRE : trouve ceux-ci de manière programmatique
add_definitions(-DCOMPILATEUR_C_COULISSE_C="/usr/bin/gcc-10")
add_definitions(-DCOMPILATEUR_CXX_COULISSE_C="/usr/bin/g++-10")

# ------------------------------------------------------------------------------

set(RACINE_INSTALLATION kuri)

include(CTest)
enable_testing()

# Macro pour compiler un fichier d'entête C en un fichier .kuri pour pouvoir appeler les fonctions
# y définis dans les programmes Kuri.
macro(compile_interface_c __nom_projet__ __nom_bibliotheque__ __fichier_entete__ __nom_module__ __nom_fichier_genere__)
  # Attend que Parcer est compilé.
  add_dependencies(${__nom_projet__} parcer)

  # Niveau d'indirection pour générer un fichier .kuri et l'installer puisque CMake ne connait pas
  # ce genre de fichier
  set(FICHIER_MODULE "")

  set(FICHIER_DESTINATION "/tmp/${__nom_fichier_genere__}.kuri")
  set(FICHIER_ENTETE "${CMAKE_CURRENT_SOURCE_DIR}/${__fichier_entete__}")

  add_custom_command(
      OUTPUT ${FICHIER_DESTINATION}
      COMMAND parcer "${FICHIER_ENTETE}" -b "${__nom_bibliotheque__}" -o "${FICHIER_DESTINATION}"
      DEPENDS parcer "${FICHIER_ENTETE}"
      COMMENT "Génération de l'interface Kuri pour ${FICHIER_ENTETE}"
  )

  list(APPEND FICHIER_MODULE ${FICHIER_DESTINATION})

  # Attend que FICHIER_MODULE est disponible
  add_custom_target(SOURCE_KURI_MODULE_${__nom_projet__} ALL DEPENDS "${FICHIER_MODULE}")

  install(
      FILES "${FICHIER_MODULE}"
      DESTINATION ${RACINE_INSTALLATION}/modules/${__nom_module__}/)
endmacro(compile_interface_c)

# Compile un module C en une bibliothèque utilisable depuis le langage.
# Ceci compile une version statique, et une version dynamique de la bibliothèque, ainsi que l'interface Kuri pour le module.
macro(compile_module_c __nom__ __nom_interface__ __entete_interface__ __dossier__ __sources__ __inclusions__ __bibliotheques__)
	set(NOM_STATIQUE ${__nom__}_statique)
	set(NOM_DYNAMIQUE ${__nom__}_dynamique)

	set(CHEMIN_INSTALLATION ${RACINE_INSTALLATION}/modules/${__dossier__}/lib/x86_64-linux-gnu/)

	# Version statique
	add_library(${NOM_STATIQUE} STATIC ${__sources__})
	target_include_directories(${NOM_STATIQUE} PUBLIC ${__inclusions__})
	target_link_libraries(${NOM_STATIQUE} ${__bibliotheques__})
	set_target_properties(${NOM_STATIQUE} PROPERTIES OUTPUT_NAME ${__nom__})
	install(TARGETS ${NOM_STATIQUE} ARCHIVE DESTINATION ${CHEMIN_INSTALLATION})

	# Version dynamique
	add_library(${NOM_DYNAMIQUE} SHARED ${__sources__})
	target_include_directories(${NOM_DYNAMIQUE} PUBLIC ${__inclusions__})
	target_link_libraries(${NOM_DYNAMIQUE} ${__bibliotheques__})
	set_target_properties(${NOM_DYNAMIQUE} PROPERTIES OUTPUT_NAME ${__nom__})
	install(TARGETS ${NOM_DYNAMIQUE} LIBRARY DESTINATION ${CHEMIN_INSTALLATION})

	# Compile l'interface, nous devons dépendre sur une cible de compilation,
	# prenons arbitrairement la cible statique
	compile_interface_c(${NOM_STATIQUE} ${__nom__} ${__entete_interface__} ${__dossier__} interface_${__nom_interface__})
endmacro()

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
    exemples/applications/chemin_complet.kuri
    exemples/applications/cliente.kuri
    exemples/applications/crée_test_compilateur.kuri
    exemples/applications/json.kuri
    exemples/applications/labyrinthe.kuri
    exemples/applications/serveuse.kuri
    exemples/applications/serveuse_ssl.kuri
    exemples/applications/sudoku.kuri
    exemples/demos/demo_ajoute_init_fini.kuri
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
    exemples/tests/test_structure_poly.kuri
    exemples/tests/test_sys_fichier.kuri
    exemples/tests/test_unicode.kuri
    exemples/tests/test_valeur_polymorphique_fonction.kuri
    exemples/tests/tous_les_modules.kuri
    modules/GHTML/test/test_ghtml.kuri
    modules/HTML/Autres/entropisation.kuri
    modules/HTML/Autres/formattage.kuri
    modules/HTML/Autres/minification.kuri
    modules/HTML/Autres/stats_parsage.kuri
    modules/HTML/Autres/vérification.kuri
    modules/LABT/Programmes/principale.kuri
    modules/OutilsMaintenance/Programmes/formattage.kuri
    modules/OutilsMaintenance/Programmes/suppression_lignes_vides.kuri
    modules/OutilsMaintenance/Programmes/triage_imports.kuri
)

foreach(FICHIER_TEST ${FICHIERS_TESTS})
    string(REPLACE "/" "_" nom_test_pour_fichier ${FICHIER_TEST})
    add_test(NAME ${nom_test_pour_fichier} COMMAND kuri ${CMAKE_CURRENT_SOURCE_DIR}/${FICHIER_TEST} --tests)
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

if(CHEMIN_DELSACE_PRIVE)
  include(${CHEMIN_DELSACE_PRIVE}/CMakeLists.txt)
endif()
