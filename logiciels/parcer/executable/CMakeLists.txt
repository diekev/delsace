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

set(NOM_PROJET parcer)

find_package(LLVM REQUIRED)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${INCLUSIONS_DELSACE_CHRONO}
	/usr/lib/llvm-6.0/include/
)

set(BIBLIOTHEQUES
	stdc++fs
	bib_outils
	/usr/lib/llvm-6.0/lib/libclangTooling.a
	/usr/lib/llvm-6.0/lib/libclangFrontend.a
	/usr/lib/llvm-6.0/lib/libclangFrontendTool.a
	/usr/lib/llvm-6.0/lib/libclangDriver.a
	/usr/lib/llvm-6.0/lib/libclangSerialization.a
	/usr/lib/llvm-6.0/lib/libclangCodeGen.a
	/usr/lib/llvm-6.0/lib/libclangParse.a
	/usr/lib/llvm-6.0/lib/libclangSema.a
	/usr/lib/llvm-6.0/lib/libclangStaticAnalyzerFrontend.a
	/usr/lib/llvm-6.0/lib/libclangStaticAnalyzerCheckers.a
	/usr/lib/llvm-6.0/lib/libclangStaticAnalyzerCore.a
	/usr/lib/llvm-6.0/lib/libclangAnalysis.a
	/usr/lib/llvm-6.0/lib/libclangARCMigrate.a
	/usr/lib/llvm-6.0/lib/libclangRewrite.a
	/usr/lib/llvm-6.0/lib/libclangRewriteFrontend.a
	/usr/lib/llvm-6.0/lib/libclangEdit.a
	/usr/lib/llvm-6.0/lib/libclangAST.a
	/usr/lib/llvm-6.0/lib/libclangLex.a
	/usr/lib/llvm-6.0/lib/libclangBasic.a
	/usr/lib/llvm-6.0/lib/libLLVM.so
	/usr/lib/llvm-6.0/lib/libclang.so
	bib_json
)

add_executable(${NOM_PROJET}
    lanceur.cc
)

get_target_property(EXTLIB_COMPILE_FLAGS ${NOM_PROJET} COMPILE_OPTIONS)
list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wundef)
list(APPEND EXTLIB_COMPILE_FLAGS -Wno-comment)
set_target_properties(${NOM_PROJET} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION .
)
