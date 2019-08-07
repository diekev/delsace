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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# ------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE_INIT "Release")

# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fconcepts)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_compile_options(-g)
	add_compile_options(-Og)
	add_compile_options(-Wall)
	add_compile_options(-Wextra)
	add_compile_options(-Werror)
	# Standard
	add_compile_options(-Wpedantic)
	add_compile_options(-Weffc++)
	# Déclaration de paramètres
	add_compile_options(-Wshadow)
	add_compile_options(-Wno-unused-parameter)
	add_compile_options(-Wunused)
	# Transtypage
	add_compile_options(-Wold-style-cast)
	add_compile_options(-Wcast-align)
	add_compile_options(-Wconversion)
	add_compile_options(-Wsign-conversion)
	add_compile_options(-Wdouble-promotion)
	add_compile_options(-Wuseless-cast)
	# Orientation objet
	add_compile_options(-Wnon-virtual-dtor)
	add_compile_options(-Woverloaded-virtual)
	add_compile_options(-Wno-delete-non-virtual-dtor)
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
	add_compile_options(-fno-nonansi-builtins)
	add_compile_options(-Warray-bounds)
	add_compile_options(-fdiagnostics-color=always)
	add_compile_options(-fsanitize=address)
	add_compile_options(-fuse-ld=gold)
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

add_compile_options(-march=native)

# ------------------------------------------------------------------------------

option(AVEC_ARACHNE "Compiler le logiciel Arachné" ON)
option(AVEC_CRESUS "Compiler Crésus" ON)
option(AVEC_DANJO "Compiler le langage de création d'interface Danjo" ON)
option(AVEC_HIKAGE "Compiler Hikage" ON)
option(AVEC_HIMITSU "Compiler Himitsu" ON)
option(AVEC_IMAGO "Compiler Imago" ON)
option(AVEC_INFOGRAPHIE "Compiler Infographie" ON)
option(AVEC_KANBA "Compiler le logiciel de peinture 3D Kanba" ON)
option(AVEC_KUMO "Compiler le langage de création de tables MySQL Kumo" ON)
option(AVEC_KURI "Compiler le compilateur du langage Kuri" ON)
option(AVEC_KYANBASU "Compiler Kyanbasu" ON)
option(AVEC_MESURE "Compiler le langage de statistiques de lignes de code Mesure" ON)
option(AVEC_MIKISA "Compiler le logiciel de création 3D et compositing Mikisa" ON)
option(AVEC_PARCER "Compiler le parser C/C++ Parcer" ON)
option(AVEC_SILVATHEQUE "Compiler le logiciel de création d'arbre 3D Silvathèque" ON)
option(AVEC_TORI "Compiler le compilateur du langage Tori" ON)
option(AVEC_TORI_V2 "Compiler Tori V2" ON)
option(AVEC_VULKAN "Compiler le logiciel test de Vulkan" ON)

option(SEULEMENT_KURI "Ne compiler que Kuri et ses dépendances" OFF)

# Évite de polluer les options pour l'instant.
set(REQUIERS_QT OFF)
set(REQUIERS_TBB ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_OPENGL ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_BOOST ON)
set(REQUIERS_DOCOPT ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_KELVINLET OFF)
set(REQUIERS_PATATE OFF)
set(REQUIERS_SNH OFF)
set(REQUIERS_VORO OFF)

if(SEULEMENT_KURI)
	set(AVEC_KURI ON)

	set(AVEC_ARACHNE OFF)
	set(AVEC_CRESUS OFF)
	set(AVEC_DANJO OFF)
	set(AVEC_HIKAGE OFF)
	set(AVEC_HIMITSU OFF)
	set(AVEC_IMAGO OFF)
	set(AVEC_INFOGRAPHIE OFF)
	set(AVEC_KANBA OFF)
	set(AVEC_KUMO OFF)
	set(AVEC_KYANBASU OFF)
	set(AVEC_MESURE OFF)
	set(AVEC_MIKISA OFF)
	set(AVEC_PARCER OFF)
	set(REQUIERS_QT OFF)
	set(AVEC_SILVATHEQUE OFF)
	set(AVEC_TORI OFF)
	set(AVEC_TORI_V2 OFF)
	set(AVEC_VULKAN OFF)
endif(SEULEMENT_KURI)

macro(ajoute_sousdossier __test__ __nom__)
	if (${__test__})
		add_subdirectory(${__nom__})
	endif()
endmacro()

# À FAIRE : permet la déclaration de plusiers dépendances
macro(active_dependances __test__ __dep__)
	if (${__test__})
		set(${__dep__} ON)
	endif()
endmacro()

active_dependances(AVEC_KANBA AVEC_DANJO)
active_dependances(AVEC_MIKISA AVEC_DANJO)
active_dependances(AVEC_SILVATHEQUE AVEC_DANJO)

active_dependances(AVEC_MIKISA REQUIERS_TBB)

active_dependances(AVEC_KANBA REQUIERS_OPENGL)
active_dependances(AVEC_MIKISA REQUIERS_OPENGL)
active_dependances(AVEC_SILVATHEQUE REQUIERS_OPENGL)
active_dependances(AVEC_VULKAN REQUIERS_OPENGL)

active_dependances(AVEC_MIKISA REQUIERS_KELVINLET)
active_dependances(AVEC_MIKISA REQUIERS_PATATE)
active_dependances(AVEC_MIKISA REQUIERS_SNH)
active_dependances(AVEC_MIKISA REQUIERS_VORO)

active_dependances(AVEC_DANJO REQUIERS_QT)

# ------------------------------------------------------------------------------

if(AVEC_KANBA)
	find_package(PTex REQUIRED)
endif()

if(AVEC_MIKISA)
	find_package(Alembic)
	find_package(Bullet)
	find_package(OpenColorIO)
	find_package(OpenEXR)
	find_package(OpenSubDiv)
	find_package(OpenVDB)
endif(AVEC_MIKISA)

if(REQUIERS_QT)
	find_package(Qt5Core REQUIRED)
	set(INCLUSIONS_QT5_CORE ${Qt5Core_INCLUDE_DIRS})
	set(BIBLIOTHEQUES_QT5_CORE Qt5::Core)

	find_package(Qt5Gui REQUIRED)
	set(INCLUSIONS_QT5_GUI ${Qt5Gui_INCLUDE_DIRS})
	set(BIBLIOTHEQUES_QT5_GUI Qt5::Gui)

	find_package(Qt5OpenGL REQUIRED)
	set(INCLUSIONS_QT5_OPENGL ${Qt5OpenGL_INCLUDE_DIRS})
	set(BIBLIOTHEQUES_QT5_OPENGL Qt5::OpenGL)

	find_package(Qt5Widgets REQUIRED)
	set(INCLUSIONS_QT5_WIDGETS ${Qt5Widgets_INCLUDE_DIRS})
	set(BIBLIOTHEQUES_QT5_WIDGETS Qt5::Widgets)

	find_package(Qt5Sql REQUIRED)
	set(INCLUSIONS_QT5_SQL ${Qt5Sql_INCLUDE_DIRS})
	set(BIBLIOTHEQUES_QT5_SQL Qt5::Sql)

	set(INCLUSIONS_QT "${INCLUSIONS_QT5_CORE}" "${INCLUSIONS_QT5_GUI}" "${INCLUSIONS_QT5_OPENGL}" "${INCLUSIONS_QT5_WIDGETS}" "${INCLUSIONS_QT5_SQL}")
	set(BIBLIOTHEQUES_QT "${BIBLIOTHEQUES_QT5_CORE}" "${BIBLIOTHEQUES_QT5_GUI}" "${BIBLIOTHEQUES_QT5_OPENGL}" "${BIBLIOTHEQUES_QT5_WIDGETS}" "${BIBLIOTHEQUES_QT5_SQL}")
endif()

# À FAIRE : find_package
if(REQUIERS_OPENGL)
	set(BIBLIOTHEQUES_OPENGL GLEW GLU GL glut)
endif()

# À FAIRE : find_package
if(REQUIERS_TBB)
	set(BIBLIOTHEQUES_TBB tbb)
endif()

set(BIBLIOTHEQUES_IMAGE ${BIBLIOTHEQUES_IMAGE} jpeg png)

# À FAIRE : find_package
if(REQUIERS_BOOST)
	set(INCLUSIONS_BOOST /opt/lib/boost/include)
endif()

# ------------------------------------------------------------------------------

add_subdirectory(biblexternes)
add_subdirectory(biblinternes)
add_subdirectory(logiciels)
add_subdirectory(tests)
