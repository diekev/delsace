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
)

set(SOURCES
	analyseuse.h
	analyseuse_grammaire.h
	arbre_syntactic.h
	assembleuse_arbre.h
    decoupeuse.h
	erreur.h
	expression.h
	morceaux.h
	nombres.h
	tampon_source.h
	unicode.h

	analyseuse.cc
	analyseuse_grammaire.cc
	arbre_syntactic.cc
	assembleuse_arbre.cc
    decoupeuse.cc
	erreur.cc
	expression.cc
	morceaux.cc
	nombres.cc
	preproces.cc
	tampon_source.cc
	unicode.cc
)

add_library(decoupage STATIC "${SOURCES}")
target_include_directories(decoupage PUBLIC "${INCLUSIONS}")

