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

project(danjo)

# ------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)

add_compile_options(-fconcepts)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -Og -Wno-error=unused-function -Wextra -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unknown-pragmas -Wno-missing-field-initializers -Wno-sign-compare -Wno-type-limits -Wmissing-format-attribute -fno-common -Wundef -Wno-delete-non-virtual-dtor -Wno-div-by-zero  -Wstrict-aliasing=2 -Wsizeof-pointer-memaccess -Wformat=2 -Wno-format-nonliteral -Wno-format-y2k -fstrict-overflow -Wstrict-overflow=2 -Wwrite-strings -Wformat-extra-args -Wlogical-op -Wnonnull -fno-omit-frame-pointer -Wno-error=unused-result -Wno-error=clobbered -fstack-protector-all --param=ssp-buffer-size=4 -Werror -Wno-maybe-uninitialized -Wmissing-include-dirs -Wdate-time -Wuninitialized -Winit-self -Wtype-limits -fno-nonansi-builtins -Warray-bounds -Wno-error=unused-local-typedefs -fdiagnostics-color=always -fsanitize=address -fuse-ld=gold")

# ------------------------------------------------------------------------------

find_package(Delsace COMPONENTS math tests REQUIRED)

find_package(Qt5Core REQUIRED)
set(INCLUSIONS_QT5_CORE ${Qt5Core_INCLUDE_DIRS})
set(BIBLIOTHEQUES_QT5_CORE Qt5::Core)

find_package(Qt5Gui REQUIRED)
set(INCLUSIONS_QT5_GUI ${Qt5Gui_INCLUDE_DIRS})
set(BIBLIOTHEQUES_QT5_GUI Qt5::Gui)

find_package(Qt5Widgets REQUIRED)
set(INCLUSIONS_QT5_WIDGETS ${Qt5Widgets_INCLUDE_DIRS})
set(BIBLIOTHEQUES_QT5_WIDGETS Qt5::Widgets)

add_compile_options(-fPIC)

# ------------------------------------------------------------------------------

set(INCLUSIONS_SYSTEMES
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUSIONS_QT5_CORE}
	${INCLUSIONS_QT5_GUI}
	${INCLUSIONS_QT5_WIDGETS}
)

set(BIBLIOTHEQUES
	${BIBLIOTHEQUES_QT5_CORE}
	${BIBLIOTHEQUES_QT5_GUI}
	${BIBLIOTHEQUES_QT5_WIDGETS}

	stdc++fs
)

# ------------------------------------------------------------------------------

add_subdirectory(coeur)
add_subdirectory(exemples)
add_subdirectory(test)
