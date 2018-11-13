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

# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.8)

# ------------------------------------------------------------------------------

project(delsace)

# ------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_compile_options(-g)
	add_compile_options(-Og)
	add_compile_options(-Wall)
	add_compile_options(-Wextra)
	add_compile_options(-Werror)
	# Standard
	add_compile_options(-Wpedantic)
	# Déclaration de paramètres
	add_compile_options(-Wshadow)
	add_compile_options(-Wno-unused-parameter)
	add_compile_options(-Wunused)
	# Transtypage
	add_compile_options(-Wcast-align)
	add_compile_options(-Wconversion)
	add_compile_options(-Wsign-conversion)
	add_compile_options(-Wdouble-promotion)
	# Désactive certaines erreurs
	add_compile_options(-Wno-error=unused-function)
	add_compile_options(-Wno-error=class-memaccess)
	add_compile_options(-Wno-error=unused-result)
	add_compile_options(-Wno-error=clobbered)
	add_compile_options(-Wno-error=unused-local-typedefs)
	# Format
	add_compile_options(-Wformat=2)
	add_compile_options(-Wno-format-nonliteral)
	add_compile_options(-Wno-format-y2k)
	add_compile_options(-Wmissing-format-attribute)
	add_compile_options(-Wformat-extra-args)
	# Pointeurs
	add_compile_options(-Wnonnull)
	add_compile_options(-Wnull-dereference)
	# Conditions
	add_compile_options(-Wduplicated-cond)
	add_compile_options(-Wduplicated-branches)
	# Operations
	add_compile_options(-Wlogical-op)
	# Autres
	#add_compile_options(-Wlifetime) non-supporté
	add_compile_options(-Wno-ignored-qualifiers)
	add_compile_options(-Wno-unknown-pragmas)
	add_compile_options(-Wno-missing-field-initializers)
	add_compile_options(-Wno-sign-compare)
	add_compile_options(-Wno-type-limits)
	add_compile_options(-fno-common)
	add_compile_options(-Wundef)
	add_compile_options(-Wno-div-by-zero)
	add_compile_options(-Wstrict-aliasing=2)
	add_compile_options(-Wsizeof-pointer-memaccess)
	add_compile_options(-fstrict-overflow)
	add_compile_options(-Wstrict-overflow=2)
	add_compile_options(-Wwrite-strings)
	add_compile_options(-fno-omit-frame-pointer)
	add_compile_options(-fstack-protector-all)
	add_compile_options(--param=ssp-buffer-size=4)
	add_compile_options(-Wno-maybe-uninitialized)
	add_compile_options(-Wmissing-include-dirs)
	add_compile_options(-Wdate-time)
	add_compile_options(-Wuninitialized)
	add_compile_options(-Winit-self)
	add_compile_options(-Wtype-limits)
	add_compile_options(-Warray-bounds)
	add_compile_options(-fdiagnostics-color=always)
	add_compile_options(-fsanitize=address)
	add_compile_options(-fuse-ld=gold)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++ -Wuseless-cast -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wno-delete-non-virtual-dtor -fno-nonansi-builtins")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fuse-ld=gold")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -fuse-ld=gold")
elseif(CMAKE_BUILD_TYPE MATCHES "Profile")
	add_compile_options(-g)
	add_compile_options(-pg)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
	add_compile_options(-O3)
endif()

# ------------------------------------------------------------------------------

add_subdirectory(base64)
add_subdirectory(bcrypt)
add_subdirectory(chrono)
add_subdirectory(connexion_mysql)
add_subdirectory(kumo)
add_subdirectory(parallèle)
add_subdirectory(réseau)
add_subdirectory(sha256)
add_subdirectory(tests)
add_subdirectory(tests_delsace)
add_subdirectory(tori)
add_subdirectory(unicode)
