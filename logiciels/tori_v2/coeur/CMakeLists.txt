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
	${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

set(BIBLIOTHEQUES
	langage
	bib_structures
)

add_executable(tori_v2
	analyseuse_langage.h
    decoupeur.h
	erreur.h
    morceaux.h
	postfix.h

	analyseuse_langage.cc
    decoupeur.cc
	erreur.cc
    morceaux.cc
	postfix.cc
    test.cc
)

target_include_directories(tori_v2 PUBLIC "${INCLUSIONS}")
target_link_libraries(tori_v2 "${BIBLIOTHEQUES}")
