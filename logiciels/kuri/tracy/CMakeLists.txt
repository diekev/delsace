# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2023 Kévin Dietrich.

set(NOM_CIBLE kuri_tracy)

set(SOURCES
    TracyClient.cpp
)

add_library(${NOM_CIBLE} STATIC "${SOURCES}")
add_library(kuri::tracy ALIAS kuri_tracy)

set_property(TARGET ${NOM_CIBLE} PROPERTY POSITION_INDEPENDENT_CODE ON)
desactive_drapeaux_compilation(${NOM_CIBLE})
