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
	${INCLUSIONS_SYSTEMES}
)

qt5_wrap_cpp(
	moc_headers

	interne/action.h
	interne/base_controles.h
	interne/bouton.h
	interne/controles.h

	conteneur_controles.h
)

set(ENTETES
	conteneur_controles.h
	erreur.h
	kangao.h
	manipulable.h
	repondant_bouton.h
)

add_library(kangao STATIC
	interne/action.cc
	interne/analyseur.cc
	interne/assembleur_disposition.cc
	interne/base_controles.cc
	interne/bouton.cc
	interne/controles.cc
	interne/decoupeur.cc
	interne/donnees_controle.cc
	interne/morceaux.cc

	conteneur_controles.cc
	erreur.cc
	kangao.cc
	manipulable.cc
	repondant_bouton.cc

	interne/action.h
	interne/analyseur.h
	interne/assembleur_disposition.h
	interne/base_controles.h
	interne/bouton.h
	interne/controles.h
	interne/decoupeur.h
	interne/donnees_controle.h
	interne/morceaux.h

	${ENTETES}

	${moc_headers}
)

target_include_directories(kangao PUBLIC "${INCLUSIONS}")

target_link_libraries(kangao "${BIBLIOTHEQUES}")

install(TARGETS kangao ARCHIVE DESTINATION lib)

install(
	FILES ${ENTETES}
	DESTINATION include/kangao
)
