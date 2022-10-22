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
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${INCLUSIONS_DELSACE_CHRONO}
)

################################################################################

set(NOM_PROJET kuri)

set(BIBLIOTHEQUES
	kuri_compilation
	kuri_parsage
	kuri_representation_intermediaire
	stdc++fs
	bib_outils
	systeme_fichier
)

add_executable(kuri
    lanceur.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
	TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

install(FILES fichiers/execution_kuri.S DESTINATION ${RACINE_INSTALLATION}/fichiers)
install(FILES fichiers/point_d_entree.c DESTINATION ${RACINE_INSTALLATION}/fichiers)

################################################################################

add_executable(kuri_markov
	kuri_markov.cc
)

target_include_directories(kuri_markov PUBLIC "${INCLUSIONS}")

target_link_libraries(kuri_markov "${BIBLIOTHEQUES}")

install(
	TARGETS kuri_markov RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

add_executable(kuri_remplace
	kuri_remplace.cc
)

target_include_directories(kuri_remplace PUBLIC "${INCLUSIONS}")

target_link_libraries(kuri_remplace "${BIBLIOTHEQUES}" bib_json)

install(
	TARGETS kuri_remplace RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

add_executable(demon_kuri
	demon.cc
)

target_include_directories(demon_kuri PUBLIC "${INCLUSIONS}")

target_link_libraries(demon_kuri "${BIBLIOTHEQUES}")

install(
	TARGETS demon_kuri RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

if(AVEC_LLVM)
	set(NOM_PROJET parcer)

	set(INCLUSIONS
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../
		${INCLUSIONS_DELSACE_CHRONO}
			${CLANG_INCLUDE_DIR}
	)

	set(BIBLIOTHEQUES
		stdc++fs
		bib_outils
			/usr/lib/llvm-12/lib/libclangTooling.a
			/usr/lib/llvm-12/lib/libclangFrontend.a
			/usr/lib/llvm-12/lib/libclangFrontendTool.a
			/usr/lib/llvm-12/lib/libclangDriver.a
			/usr/lib/llvm-12/lib/libclangSerialization.a
			/usr/lib/llvm-12/lib/libclangCodeGen.a
			/usr/lib/llvm-12/lib/libclangParse.a
			/usr/lib/llvm-12/lib/libclangSema.a
			/usr/lib/llvm-12/lib/libclangStaticAnalyzerFrontend.a
			/usr/lib/llvm-12/lib/libclangStaticAnalyzerCheckers.a
			/usr/lib/llvm-12/lib/libclangStaticAnalyzerCore.a
			/usr/lib/llvm-12/lib/libclangAnalysis.a
			/usr/lib/llvm-12/lib/libclangARCMigrate.a
			/usr/lib/llvm-12/lib/libclangRewrite.a
			/usr/lib/llvm-12/lib/libclangRewriteFrontend.a
			/usr/lib/llvm-12/lib/libclangEdit.a
			/usr/lib/llvm-12/lib/libclangAST.a
			/usr/lib/llvm-12/lib/libclangLex.a
			/usr/lib/llvm-12/lib/libclangBasic.a
			/usr/lib/llvm-12/lib/libLLVM.so
			/usr/lib/llvm-12/lib/libclang.so
		bib_json
			kuri_structures
	)

	add_executable(${NOM_PROJET}
		parcer.cc
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
endif()
