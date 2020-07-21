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

set(NOM_PROJET krypto)

enable_language(ASM)

set(CMAKE_ASM_FLAGS "${CXXFLAGS} -x assembler-with-cpp")

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

set(BIBLIOTHEQUES
)

add_library(${NOM_PROJET} STATIC
	bcrypt.c
	crypt_blowfish.c
	crypt_gensalt.c
	krypto.cc
	wrapper.c

	crc32.cpp
	#digest.cpp
	#keccak.cpp
	md5.cpp
	sha1.cpp
	sha3.cpp
    sha256.cpp

    crc32.h
    hash.h
    hmac.h
    keccak.h
    md5.h
    sha1.h
    sha3.h
    sha256.h

	bcrypt.h
	crypt.h
	crypt_blowfish.h
	crypt_gensalt.h
	krypto.hh
	ow-crypt.h

	x86.S
)

get_target_property(EXTLIB_COMPILE_FLAGS ${NOM_PROJET} COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=class-memaccess)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-error=deprecated-copy)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Weffc++)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wold-style-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wuseless-cast)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wnon-virtual-dtor)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Woverloaded-virtual)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wno-delete-non-virtual-dtor)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -fconcepts)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -fno-nonansi-builtins)
set_target_properties(${NOM_PROJET} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET}
	ARCHIVE
	DESTINATION /opt/bin/kuri/modules/Krypto/)

install(
	FILES krypto.hh
	DESTINATION /opt/bin/kuri/modules/Krypto/)
