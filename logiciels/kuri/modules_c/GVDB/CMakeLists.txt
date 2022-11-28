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

find_package(CUDA REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(NOM_PROJET module_c_gvdb)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
  ${CUDA_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIR}
)

set(BIBLIOTHEQUES
  ${OPENGL_LIBRARIES}
)

set(SOURCES_GVDB
  #src/gvdb_cutils.cu
  #src/gvdb_cutils.cuh
  src/app_perf.cpp
  src/gvdb_allocator.cpp
  src/gvdb_camera.cpp
  src/gvdb_model.cpp
  src/gvdb_node.cpp
  src/gvdb_render_opengl.cpp
  src/gvdb_scene.cpp
  src/gvdb_types.cpp
  src/gvdb_vec.cpp
  src/gvdb_volume_3D.cpp
  src/gvdb_volume_base.cpp
  src/gvdb_volume_gvdb.cpp
  src/loader_ObjarReader.cpp
  src/loader_OBJReader.cpp
  src/loader_Parser.cpp
  src/string_helper.cpp
  src/app_perf.h
  src/gvdb.h
  src/gvdb_allocator.h
  src/gvdb_camera.h
  src/gvdb_model.h
  src/gvdb_node.h
  src/gvdb_render.h
  src/gvdb_scene.h
  src/gvdb_types.h
  src/gvdb_vec.h
  src/gvdb_volume_3D.h
  src/gvdb_volume_base.h
  src/gvdb_volume_gvdb.h
  src/loader_ObjarReader.h
  src/loader_OBJReader.h
  src/loader_Parser.h
  src/string_helper.h
)

set(SOURCES
  ${SOURCES_GVDB}

  gvdb_capi.cc
  gvdb_capi.h
)

compile_module_c(${NOM_PROJET} gvdb gvdb_capi.h GVDB "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")

desactive_drapeaux_compilation_c(${NOM_PROJET}_statique)
desactive_drapeaux_compilation_c(${NOM_PROJET}_dynamique)

target_compile_definitions(${NOM_PROJET}_statique PRIVATE BUILD_OPENGL GVDB_EXPORTS)
target_compile_definitions(${NOM_PROJET}_dynamique PRIVATE BUILD_OPENGL GVDB_EXPORTS)

enable_language(CUDA)

# Compile a couple of source files into a PTX object library.
# Creates an object library target named gvdbPTX.
set(GVDB_PTX_SOURCE_FILES "kernels/cuda_gvdb_copydata.cu" "kernels/cuda_gvdb_module.cu")
add_library(gvdbPTX OBJECT ${GVDB_PTX_SOURCE_FILES})
set_target_properties(gvdbPTX PROPERTIES CUDA_PTX_COMPILATION ON)

# Allow GVDB to use e.g. CUDA_GVDB_COPYDATA as a macro for "cuda_gvdb_copydata.ptx"
foreach(_FILE ${GVDB_PTX_SOURCE_FILES})
    get_filename_component(_FILE ${_FILE} NAME) # cuda_gvdb_copydata.cu
    string(REPLACE ".cu" ".ptx" _FILE ${_FILE}) # cuda_gvdb_copydata.ptx
    string(REPLACE "." "_" _MACRO ${_FILE})     # cuda_gvdb_copydata_ptx
    string(TOUPPER ${_MACRO} _MACRO) # Yep, this goes input, then output!
    target_compile_definitions(${NOM_PROJET}_statique PUBLIC ${_MACRO}="${_FILE}")
    target_compile_definitions(${NOM_PROJET}_dynamique PUBLIC ${_MACRO}="${_FILE}")
endforeach()

# Enable the --use_fast_math CUDA compiler flag.
target_compile_options(gvdbPTX PRIVATE --use_fast_math)
desactive_drapeaux_compilation_pour_nvcc(gvdbPTX)

# Export our additional target include directories.
target_include_directories(gvdbPTX
    INTERFACE   $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/kernels>
                $<INSTALL_INTERFACE:include>)
