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
option(AVEC_MODULES_C "Active la génération des modules implémentés en C" OFF)
option(AVEC_ALEMBIC "Active la génération du module Alembic" OFF)
option(AVEC_IMAGE "Active la génération du module Image" OFF)
option(AVEC_OPENVDB "Active la génération du module OpenVDB" OFF)

if(AVEC_LLVM)
	find_package(LLVM REQUIRED)
	add_definitions(-DAVEC_LLVM)
endif()

find_program(GPERF NAME gperf)

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

macro(definis_bibliotheque __statique_ou_partagee__ __nom__ __nom_cible__ __sources__ __inclusions__ __bibliotheques__)
    add_library(${__nom_cible__} ${__statique_ou_partagee__} ${__sources__})
    target_include_directories(${__nom_cible__} PUBLIC ${__inclusions__})
    target_link_libraries(${__nom_cible__} ${__bibliotheques__})
    set_target_properties(${__nom_cible__} PROPERTIES OUTPUT_NAME ${__nom__})
    install(TARGETS ${__nom_cible__} DESTINATION ${CHEMIN_INSTALLATION})
endmacro()

# Compile un module C en une bibliothèque utilisable depuis le langage.
# Ceci compile une version statique, et une version dynamique de la bibliothèque, ainsi que l'interface Kuri pour le module.
macro(compile_module_c __nom__ __nom_interface__ __entete_interface__ __dossier__ __sources__ __inclusions__ __bibliotheques__)
    set(NOM_STATIQUE ${__nom__}_statique)
    set(NOM_DYNAMIQUE ${__nom__}_dynamique)

    set(CHEMIN_INSTALLATION ${RACINE_INSTALLATION}/modules/${__dossier__}/lib/x86_64-linux-gnu/)

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(NOM ${__nom__}_debogage)
    elseif(CMAKE_BUILD_TYPE MATCHES "Profile")
        set(NOM ${__nom__}_profile)
    else()
        set(NOM ${__nom__})
    endif()

    definis_bibliotheque(STATIC ${NOM} ${NOM_STATIQUE} "${__sources__}" "${__inclusions__}" "${__bibliotheques__}")
    definis_bibliotheque(SHARED ${NOM} ${NOM_DYNAMIQUE} "${__sources__}" "${__inclusions__}" "${__bibliotheques__}")
    set_property(TARGET ${NOM_DYNAMIQUE} PROPERTY POSITION_INDEPENDENT_CODE ON)

    # Compile l'interface, nous devons dépendre sur une cible de compilation,
    # prenons arbitrairement la cible statique
    compile_interface_c(${NOM_STATIQUE} ${__nom__} ${__entete_interface__} ${__dossier__} interface_${__nom_interface__})
endmacro()

add_subdirectory(adn)
add_subdirectory(arbre_syntaxique)
add_subdirectory(compilation)
add_subdirectory(executable)
add_subdirectory(exemples)
add_subdirectory(parsage)
add_subdirectory(representation_intermediaire)
add_subdirectory(statistiques)
add_subdirectory(structures)
add_subdirectory(tests)

if(AVEC_MODULES_C)
    add_subdirectory(modules_c/Aléa)
    add_subdirectory(modules_c/Compression)
    add_subdirectory(modules_c/Exétron)
    add_subdirectory(modules_c/FTGL)
    add_subdirectory(modules_c/Géométrie3D)
    add_subdirectory(modules_c/Krypto)

    if(AVEC_ALEMBIC)
       add_subdirectory(modules_c/Alembic)
    endif()

    if(AVEC_IMAGE)
        add_subdirectory(modules_c/Image)
    endif()

    if(AVEC_OPENVDB)
        add_subdirectory(modules_c/openvdb)
    endif()
endif()


install(
	DIRECTORY modules
	DESTINATION ${RACINE_INSTALLATION}
)

##############################################################"

macro(compile_fichier __nom_dossier__ __nom_fichier__)
    string(REPLACE "/" "_" NOM_DOSSIER ${__nom_dossier__})
    # Utilise CMAKE_CURRENT_LIST_DIR puisque ce fichier est inclus depuis un dossier n'étant pas parent.
    add_test(NAME compilation_${NOM_DOSSIER}_${__nom_fichier__} COMMAND timeout 10 kuri ${CMAKE_CURRENT_LIST_DIR}/${__nom_dossier__}/${__nom_fichier__}.kuri)
endmacro()

macro(compile_fichier_avec_erreur __nom_dossier__ __nom_fichier__)
    string(REPLACE "/" "_" NOM_DOSSIER ${__nom_dossier__})
    # Utilise CMAKE_CURRENT_LIST_DIR puisque ce fichier est inclus depuis un dossier n'étant pas parent.
    add_test(NAME compilation_${NOM_DOSSIER}_${__nom_fichier__} COMMAND timeout 10 kuri ${CMAKE_CURRENT_LIST_DIR}/${__nom_dossier__}/${__nom_fichier__}.kuri)
    set_tests_properties(compilation_${NOM_DOSSIER}_${__nom_fichier__} PROPERTIES WILL_FAIL TRUE)
endmacro()

macro(compile_fichier_principale __nom_dossier__)
    compile_fichier(${__nom_dossier__} principale)
endmacro()

macro(execute_fichier __nom_dossier__ __nom_fichier__)
    compile_fichier(${__nom_dossier__} ${__nom_fichier__})
    add_test(NAME exécution_${__nom_dossier__}_${__nom_fichier__} COMMAND timeout 10 ${CMAKE_CURRENT_LIST_DIR}/${__nom_dossier__}/a.out)
endmacro()

# À FAIRE : exemples/applications/jeu_chaos.kuri
# À FAIRE : exemples/tests test_fichier.kuri
compile_fichier_principale(exemples/applications/BDD)
compile_fichier_principale(exemples/applications/EditeurTexte)
compile_fichier_principale(exemples/applications/Eliza)
compile_fichier_principale(exemples/applications/Jeu)
compile_fichier_principale(exemples/applications/NASA)

compile_fichier(exemples/applications base64)
compile_fichier(exemples/applications calendrier_républicain)
compile_fichier(exemples/applications chemin_complet)
compile_fichier(exemples/applications cliente)
compile_fichier(exemples/applications crée_test_compilateur)
compile_fichier(exemples/applications json)
compile_fichier(exemples/applications labyrinthe)
compile_fichier(exemples/applications serveuse)
compile_fichier(exemples/applications serveuse_ssl)
compile_fichier(exemples/applications sudoku)
compile_fichier(exemples/demos demo_ajoute_init_fini)
compile_fichier(exemples/demos demo_exécution)
compile_fichier(exemples/demos demo_expansion_argument_variadique)
compile_fichier(exemples/demos demo_info_type_fonction)
compile_fichier(exemples/demos demo_logement)
compile_fichier(exemples/demos demo_operateur_crochet)
compile_fichier(exemples/demos demo_operateur_point)
compile_fichier(exemples/demos demo_operateur_surcharge)
compile_fichier(exemples/demos demo_pointeurs)
compile_fichier(exemples/demos demo_reference)
compile_fichier(exemples/demos demo_si)
compile_fichier(exemples/demos demo_surcharge_fonctions)
compile_fichier(exemples/demos demo_syntax_appel_uniforme)
compile_fichier(exemples/demos demo_typage)
compile_fichier(exemples/tests test_assert_dans_fonction)
compile_fichier(exemples/tests test_annotations)
compile_fichier(exemples/tests test_boucle_pour)
compile_fichier(exemples/tests test_corps_texte)
compile_fichier(exemples/tests test_chaines_littérales)
compile_fichier(exemples/tests test_couleur_terminal)
compile_fichier(exemples/tests test_dessin_texte)
compile_fichier(exemples/tests test_emploi)
compile_fichier(exemples/tests test_énum)
compile_fichier(exemples/tests test_énum_drapeau)
compile_fichier(exemples/tests test_erreur)
compile_fichier(exemples/tests test_logement)
compile_fichier(exemples/tests test_memoire)
compile_fichier(exemples/tests test_metaprogrammation)
compile_fichier(exemples/tests test_metaprogramme_imprime)
compile_fichier(exemples/tests test_moultfilage)
compile_fichier(exemples/tests test_operateur)
compile_fichier(exemples/tests test_plage_boucle)
compile_fichier(exemples/tests test_polymorphisme)
compile_fichier(exemples/tests test_ppm)
compile_fichier(exemples/tests test_structure_poly)
compile_fichier(exemples/tests test_sys_fichier)
compile_fichier(exemples/tests test_unicode)
compile_fichier(exemples/tests test_valeur_polymorphique_fonction)
compile_fichier(exemples/tests tous_les_modules)
compile_fichier(modules/GHTML/test test_ghtml)
compile_fichier(modules/HTML/Autres entropisation)
compile_fichier(modules/HTML/Autres formattage)
compile_fichier(modules/HTML/Autres minification)
compile_fichier(modules/HTML/Autres stats_parsage)
compile_fichier(modules/HTML/Autres vérification)
compile_fichier(modules/LABT/Programmes principale)
compile_fichier(modules/OutilsMaintenance/Programmes formattage)
compile_fichier(modules/OutilsMaintenance/Programmes suppression_lignes_vides)
compile_fichier(modules/OutilsMaintenance/Programmes triage_imports)

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

include(bugs/test_bugs.txt)

if(CHEMIN_DELSACE_PRIVE)
  include(${CHEMIN_DELSACE_PRIVE}/CMakeLists.txt)
endif()
