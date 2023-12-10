# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2021-2022 Kévin Dietrich.

set(NOM_CIBLE kuri_parsage)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
    kuri_statistiques
    kuri_structures
    ${BIBLIOTHEQUE_CXX_FILESYSTEM}
    langage
    dls::outils
    dls::moultfilage
    dls::structures
    dls::memoire
)

set(SOURCES
    base_syntaxeuse.cc
    gerante_chaine.cc
    identifiant.cc
    lexemes.cc
    lexeuse.cc
    modules.cc
    outils_lexemes.cc
    site_source.cc

    identifiant.def

    base_syntaxeuse.hh
    empreinte_parfaite.hh
    gerante_chaine.hh
    identifiant.hh
    lexemes.hh
    lexeuse.hh
    modules.hh
    outils_lexemes.hh
    site_source.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexemes.cc
    COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/lexemes.cc"
    DEPENDS genere_lexemes
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/lexemes.hh
    COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/lexemes.hh"
    DEPENDS genere_lexemes
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/empreinte_parfaite.hh
    COMMAND genere_lexemes "${CMAKE_CURRENT_SOURCE_DIR}/empreinte_parfaite.hh"
    DEPENDS genere_lexemes lexemes.hh
)

add_dependencies(${NOM_CIBLE} genere_lexemes)
