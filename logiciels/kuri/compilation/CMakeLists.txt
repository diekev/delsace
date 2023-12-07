# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2018-2022 Kévin Dietrich.

set(NOM_CIBLE kuri_compilation)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

macro(genere_code_adn __bibliotheque__ __generatrice__ __fichier__)
    add_library(${__bibliotheque__} ${__fichier__}.cc ${__fichier__}.hh)
    target_include_directories(${__bibliotheque__} PUBLIC "${INCLUSIONS}")
    target_link_libraries(${__bibliotheque__} kuri_parsage)
    add_dependencies(${__bibliotheque__} ${__generatrice__})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__fichier__}.cc
        COMMAND ${__generatrice__} "${CMAKE_CURRENT_SOURCE_DIR}/${__fichier__}.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/${__fichier__}.adn
        DEPENDS ${__generatrice__} ${CMAKE_CURRENT_SOURCE_DIR}/../adn/${__fichier__}.adn
    )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__fichier__}.hh
    COMMAND ${__generatrice__} "${CMAKE_CURRENT_SOURCE_DIR}/${__fichier__}.hh" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/${__fichier__}.adn
    DEPENDS ${__generatrice__} ${CMAKE_CURRENT_SOURCE_DIR}/../adn/${__fichier__}.adn
)
endmacro()

genere_code_adn(kuri_ipa_compilatrice genere_ipa ipa)
genere_code_adn(kuri_options_compilatrice genere_options options)
genere_code_adn(kuri_message_compilatrice genere_message message)
genere_code_adn(kuri_intrinseques_compilatrice genere_intrinseques intrinseques)

set(BIBLIOTHEQUES
    kuri_arbre_syntaxique
    kuri_intrinseques_compilatrice
    kuri_ipa_compilatrice
    kuri_message_compilatrice
    kuri_options_compilatrice
    kuri_parsage
    kuri_representation_intermediaire
    kuri_statistiques
    kuri_structures
    ${BIBLIOTHEQUE_CXX_FILESYSTEM}
    langage
    dls::outils
    dls::moultfilage
    dls::structures
    dls::memoire
    nombre_decimaux
)

set(SOURCES
    attente.hh
    bibliotheque.hh
    broyage.hh
    compilatrice.hh
    coulisse.hh
    coulisse_asm.hh
    coulisse_c.hh
    coulisse_mv.hh
    discrimination.hh
    environnement.hh
    erreur.h
    espace_de_travail.hh
    gestionnaire_code.hh
    graphe_dependance.hh
    interface_module_kuri.hh
    messagere.hh
    metaprogramme.hh
    monomorpheuse.hh
    monomorphisations.hh
    numerique.hh
    operateurs.hh
    programme.hh
    portee.hh
    structures.hh
    syntaxeuse.hh
    tache.hh
    tacheronne.hh
    transformation_type.hh
    typage.hh
    unite_compilation.hh
    validation_expression_appel.hh
    validation_semantique.hh

    attente.cc
    bibliotheque.cc
    broyage.cc
    compilatrice.cc
    coulisse.cc
    coulisse_asm.cc
    coulisse_c.cc
    coulisse_mv.cc
    discrimination.cc
    environnement.cc
    erreur.cc
    espace_de_travail.cc
    gestionnaire_code.cc
    graphe_dependance.cc
    interface_module_kuri.cc
    messagere.cc
    metaprogramme.cc
    monomorpheuse.cc
    monomorphisations.cc
    numerique.cc
    operateurs.cc
    programme.cc
    portee.cc
    structures.cc
    syntaxeuse.cc
    tache.cc
    tacheronne.cc
    transformation_type.cc
    typage.cc
    unite_compilation.cc
    validation_expression_appel.cc
    validation_semantique.cc
)

if(AVEC_LLVM)
    # Utilise un objet séparé pour la coulisse LLVM afin de forcer
    # l'utilisation de C++ 17 car LLVM utilise des fonctions du langage
    # qui sont déclarées comme malformées en C++ 20.
    set(COULLISE_LLVM ${NOM_CIBLE}_coulisse_llvm)
    add_library(${COULLISE_LLVM} STATIC coulisse_llvm.cc coulisse_llvm.hh)
    target_link_libraries(${COULLISE_LLVM} "${BIBLIOTHEQUES}" ${LLVM_LIBRARY})
    target_include_directories(${COULLISE_LLVM} PUBLIC "${INCLUSIONS}" ${LLVM_INCLUDE_DIR})

    set_property(TARGET ${COULLISE_LLVM} PROPERTY CXX_STANDARD 17)

    set(BIBLIOTHEQUES "${BIBLIOTHEQUES}" ${COULLISE_LLVM})
endif()

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_dependencies(${NOM_CIBLE} kuri_ipa_compilatrice)
add_dependencies(${NOM_CIBLE} kuri_options_compilatrice)
add_dependencies(${NOM_CIBLE} kuri_message_compilatrice)
add_dependencies(${NOM_CIBLE} kuri_intrinseques_compilatrice)
