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
    chrono.cc
    compteuse_cas.cc
    divers.cc
    enum_drapeau.cc
    garde_portee.cc
    log.cc
    logeuse_memoire.cc
    macros.cc
    poule_de_taches.cc
    synchrone.cc
    type_opaque.cc
    unicode.cc

    algorithmes.hh
    calcul.hh
    chrono.hh
    compteuse_cas.hh
    divers.hh
    enum_drapeau.hh
    garde_portee.hh
    log.hh
    logeuse_memoire.hh
    macros.hh
    poule_de_taches.hh
    synchrone.hh
    type_opaque.hh
    unicode.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
add_library(kuri::utilitaires ALIAS kuri_utilitaires)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
