# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2020-2022 Kévin Dietrich.

set(NOM_CIBLE kuri_representation_intermediaire)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../compilation/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${INCLUSIONS_LIBFFI}
)

set(BIBLIOTHEQUES
    kuri_arbre_syntaxique
    kuri_ipa_compilatrice
    kuri_compilation
    kuri_statistiques
    kuri::utilitaires
    dls::outils
    systeme_fichier
    ${BIBLIOTHEQUES_LIBFFI}
    ${BIBLIOTHEQUE_DL}
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/machine_virtuelle_intrinseques.cc
    COMMAND genere_intrinseques "${CMAKE_CURRENT_SOURCE_DIR}/machine_virtuelle_intrinseques.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/intrinseques.adn
    DEPENDS genere_intrinseques ${CMAKE_CURRENT_SOURCE_DIR}/../adn/intrinseques.adn
)

set(SOURCES
    analyse.cc
    bloc_basique.cc
    code_binaire.cc
    constructrice_ri.cc
    impression.cc
    instructions.cc
    machine_virtuelle.cc
    machine_virtuelle_intrinseques.cc
    optimisations.cc
    syntaxage.cc
    visite_instructions.cc

    analyse.hh
    bloc_basique.hh
    code_binaire.hh
    constructrice_ri.hh
    impression.hh
    instructions.hh
    machine_virtuelle.hh
    optimisations.hh
    syntaxage.hh
    visite_instructions.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_dependencies(${NOM_CIBLE} kuri_arbre_syntaxique)
add_dependencies(${NOM_CIBLE} kuri_ipa_compilatrice)
