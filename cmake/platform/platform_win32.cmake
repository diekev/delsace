# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2022 Kévin Dietrich.

add_compile_definitions(_USE_MATH_DEFINES)

add_compile_options(/bigobj /utf-8)

# Pile de 2 Mo.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:2097152")

# Pour supprimer des drapeaux de compilation un peu trop stricts pour certaines
# bibliothèques externes.
macro(desactive_drapeaux_compilation cible)
    # get_target_property(EXTLIB_COMPILE_FLAGS ${cible} COMPILE_OPTIONS)
    # list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS ...)
    # set_target_properties(${cible} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")
endmacro()

# Pour supprimer des drapeaux de compilation un peu trop stricts, ou réserver à
# C++, pour certaines bibliothèques externes écrites en C.
macro(desactive_drapeaux_compilation_c cible)
    # get_target_property(EXTLIB_COMPILE_FLAGS ${cible} COMPILE_OPTIONS)
    # list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS ...)
    # set_target_properties(${cible} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")
endmacro()

macro(desactive_drapeaux_compilation_pour_nvcc cible)
    # get_target_property(EXTLIB_COMPILE_FLAGS ${cible} COMPILE_OPTIONS)
    # list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS ...)
    # set_target_properties(${cible} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")
endmacro()
