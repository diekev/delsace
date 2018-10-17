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

set(NOM_PROJET service)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
)

add_library(reseau STATIC
	cliente.cc
	cliente_http.cc
	message.cc
	outils_http.cc
	serveuse.cc
	serveuse_http.cc
	uri.cc

	cliente.hh
	cliente_http.hh
	message.hh
	outils_http.hh
	serveuse.hh
	serveuse_http.hh
	uri.hh
)

set(BIBLIOTHEQUES
	base64
	bcrypt
	reseau
	sha256
	ssl
	crypto
)

add_executable(${NOM_PROJET}
    service.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION .
)

set(NOM_PROJET clientele)

add_executable(${NOM_PROJET}
	clientele.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION .
)
