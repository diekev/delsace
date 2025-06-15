# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2025 Kévin Dietrich.

set(NOM_BIBLIOTHEQUE kuri_r16)

set(SOURCES
	r16_c.h
	r16_tables.h
	r16_tables.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES})
add_library(kuri::r16 ALIAS kuri_r16)

install(
	FILES ${SOURCES}
	DESTINATION kuri/fichiers
)

