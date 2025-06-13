# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2021-2022 Kévin Dietrich.

set(NOM_CIBLE kuri_statistiques)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(BIBLIOTHEQUES
    kuri_structures
)

set(SOURCES
    statistiques.cc
    statistiques.hh
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")

target_include_directories(${NOM_CIBLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_CIBLE} "${BIBLIOTHEQUES}")
