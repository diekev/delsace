# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2023 Kévin Dietrich.

set(NOM_CIBLE kuri_utilitaires)

set(INCLUSIONS
)

set(BIBLIOTHEQUES
    kuri::structures
)

set(SOURCES
    algorithmes.cc
    calcul.cc
    compteuse_cas.cc
    garde_portee.cc
    log.cc
    macros.cc
    poule_de_taches.cc
    type_opaque.cc

    algorithmes.hh
    calcul.hh
    compteuse_cas.hh
    garde_portee.hh
    log.hh
    macros.hh
    poule_de_taches.hh
    type_opaque.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
add_library(kuri::utilitaires ALIAS kuri_utilitaires)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
