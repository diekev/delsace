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

    algorithmes.hh
    calcul.hh
    compteuse_cas.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
add_library(kuri::utilitaires ALIAS kuri_utilitaires)

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
