# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2018-2022 Kévin Dietrich.

set(NOM_CIBLE kuri_arbre_syntaxique)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
    kuri_compilation
    kuri_parsage
    kuri_statistiques
    kuri_structures
    dls::outils
    dls::moultfilage
    dls::structures
    dls::memoire
)

set(SOURCES
    allocatrice.cc
    assembleuse.cc
    canonicalisation.cc
    cas_genre_noeud.hh
    copieuse.cc
    expression.cc
    infos_types.cc
    noeud_code.cc
    noeud_expression.cc
    utilitaires.cc

    allocatrice.hh
    assembleuse.hh
    canonicalisation.hh
    copieuse.hh
    expression.hh
    infos_types.hh
    noeud_code.hh
    noeud_expression.hh
    utilitaires.hh
)

macro(genere_source __nom__)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.cc
        COMMAND genere_noeuds "${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.cc" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/noeuds.adn
        DEPENDS genere_noeuds ../adn/noeuds.adn
    )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.hh
    COMMAND genere_noeuds "${CMAKE_CURRENT_SOURCE_DIR}/${__nom__}.hh" -i ${CMAKE_CURRENT_SOURCE_DIR}/../adn/noeuds.adn
    DEPENDS genere_noeuds ../adn/noeuds.adn
)
endmacro()

genere_source(noeud_expression)
genere_source(noeud_code)
genere_source(allocatrice)
genere_source(assembleuse)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_dependencies(kuri_arbre_syntaxique genere_noeuds)
add_dependencies(${NOM_CIBLE} kuri_arbre_syntaxique)
