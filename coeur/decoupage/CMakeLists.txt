# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2018 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUSIONS_DELSACE_CHRONO}
	${INCLUSIONS_DELSACE_LANGAGE}
	${LLVM_INCLUDE_DIR}
)

set(BIBLIOTHEQUES
	/usr/lib/llvm-6.0/lib/libLLVM.so # À FAIRE
	stdc++fs
	${BIBLIOTHEQUE_DELSACE_LANGAGE}
)

set(SOURCES
	analyseuse_grammaire.h
	arbre_syntactic.h
	assembleuse_arbre.h
	broyage.hh
	contexte_generation_code.h
	coulisse_c.hh
	coulisse_llvm.hh
    decoupeuse.h
	donnees_type.h
	erreur.h
	expression.h
	info_type.hh
	modules.hh
	morceaux.h
	nombres.h

	analyseuse_grammaire.cc
	arbre_syntactic.cc
	assembleuse_arbre.cc
	broyage.cc
	contexte_generation_code.cc
	coulisse_c.cc
	coulisse_llvm.cc
    decoupeuse.cc
	donnees_type.cc
	erreur.cc
	expression.cc
	info_type.cc
	modules.cc
	morceaux.cc
	nombres.cc
)

add_library(decoupage STATIC "${SOURCES}")

target_include_directories(decoupage PUBLIC "${INCLUSIONS}")

target_link_libraries(decoupage "${BIBLIOTHEQUES}")
