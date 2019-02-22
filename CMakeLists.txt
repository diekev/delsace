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

project(creatheque)

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

# ------------------------------------------------------------------------------

option(AVEC_KAMIKAZE "Compiler le logiciel de création 3D Kamikaze" ON)
option(AVEC_KANBA "Compiler le logiciel de peinture 3D Kanba" ON)
option(AVEC_KOUDOU "Compiler le logiciel de rendu 3D Koudou" ON)
option(AVEC_MIKISA "Compiler le logiciel de compositing Mikisa" ON)
option(AVEC_POSEIDON "Compiler le logiciel de simulation de fluide Poséidon" ON)
option(AVEC_SILVATHEQUE "Compiler le logiciel de création d'arbre 3D Silvathèque" ON)

# ------------------------------------------------------------------------------

find_package(Delsace COMPONENTS math REQUIRED)
find_package(Danjo REQUIRED)
find_package(Ego REQUIRED)
find_package(FileSystem REQUIRED)
find_package(Image REQUIRED)
find_package(Math REQUIRED)
find_package(PTex REQUIRED)
find_package(TestUnitaire REQUIRED)

if(AVEC_MIKISA)
	find_package(Bullet)
	find_package(OpenColorIO)
	find_package(OpenEXR)
	find_package(OpenSubDiv)
endif(AVEC_MIKISA)

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

set(BIBLIOTHEQUES_OPENGL GLEW GLU GL glut)
set(BIBLIOTHEQUES_TBB tbb)
set(BIBLIOTHEQUES_IMAGE ${BIBLIOTHEQUES_IMAGE} jpeg png)

# ------------------------------------------------------------------------------

set(ICONES
	fichiers/icones/icone_avertissement.png
	fichiers/icones/icon_box.png
	fichiers/icones/icon_circle.png
	fichiers/icones/icon_cone.png
	fichiers/icones/icon_grid.png
	fichiers/icones/icon_icosphere.png
	fichiers/icones/icon_jump_first.png
	fichiers/icones/icon_jump_last.png
	fichiers/icones/icon_pause.png
	fichiers/icones/icon_play_forward.png
	fichiers/icones/icon_play_backward.png
	fichiers/icones/icon_step_forward.png
	fichiers/icones/icon_step_backward.png
	fichiers/icones/icon_stop.png
	fichiers/icones/icon_torus.png
	fichiers/icones/icon_tube.png
	fichiers/icones/icon_uvsphere.png
	fichiers/icones/icone_echelle.png
	fichiers/icones/icone_manipulation.png
	fichiers/icones/icone_position.png
	fichiers/icones/icone_rotation.png
	fichiers/icones/icone_echelle_active.png
	fichiers/icones/icone_manipulation_active.png
	fichiers/icones/icone_position_active.png
	fichiers/icones/icone_rotation_active.png
)

set(STYLES
	fichiers/styles/main.qss
)

add_subdirectory(bibliotheques)

if (AVEC_KAMIKAZE)
	add_subdirectory(kamikaze)

	install(
		FILES ${ICONES}
		DESTINATION kamikaze/icones
	)

    install(
		FILES ${STYLES}
		DESTINATION kamikaze/styles
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION kamikaze/texte
	)
endif (AVEC_KAMIKAZE)

if (AVEC_KANBA)
    add_subdirectory(kanba)

	install(
		FILES ${STYLES}
		DESTINATION kanba/styles
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION kanba/texte
	)
endif (AVEC_KANBA)

if (AVEC_KOUDOU)
	add_subdirectory(koudou)

	install(
		FILES ${STYLES}
		DESTINATION koudou/styles
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION koudou/texte
	)
endif (AVEC_KOUDOU)

if (AVEC_MIKISA)
	add_subdirectory(mikisa)

	install(
		FILES ${STYLES}
		DESTINATION mikisa/styles
	)

    install(
		FILES ${ICONES}
		DESTINATION mikisa/icones
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION mikisa/texte
	)
endif (AVEC_MIKISA)

if (AVEC_POSEIDON)
	add_subdirectory(poseidon)

	install(
		FILES ${ICONES}
		DESTINATION poseidon/icones
	)

    install(
		FILES ${STYLES}
		DESTINATION poseidon/styles
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION poseidon/texte
	)
endif (AVEC_POSEIDON)

if (AVEC_SILVATHEQUE)
	add_subdirectory(silvatheque)

	install(
		FILES ${STYLES}
		DESTINATION silvatheque/styles
	)

    install(
		FILES fichiers/texte/texture_texte.png
		DESTINATION silvatheque/texte
	)
endif (AVEC_SILVATHEQUE)
