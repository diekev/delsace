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
# The Original Code is Copyright (C) 2017 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_BIBLIOTHEQUE chisei)

set(ENTETES
	genetique.h
)

set(SOURCES
	genetique.cc
)

add_library(${NOM_BIBLIOTHEQUE} STATIC ${SOURCES} ${ENTETES})

install(TARGETS ${NOM_BIBLIOTHEQUE} ARCHIVE DESTINATION lib)
install(FILES ${ENTETES} DESTINATION include/delsace/${NOM_BIBLIOTHEQUE})

if(AVEC_EXEMPLES)
	set(FICHIERS_KNAPSACK
		exemples/1000_poids.txt
		exemples/1000_valeurs.txt
		exemples/10000_poids.txt
		exemples/10000_valeurs.txt
	)

    add_executable(exemple_knapsack exemples/exemple_knapsack.cc)
	target_link_libraries(exemple_knapsack chisei)
	install(TARGETS exemple_knapsack RUNTIME DESTINATION bin)
	install(FILES ${FICHIERS_KNAPSACK} DESTINATION bin)

	add_executable(exemple_chaine exemples/exemple_chaine.cc)
	target_link_libraries(exemple_chaine chisei)
	install(TARGETS exemple_chaine RUNTIME DESTINATION bin)
endif()
