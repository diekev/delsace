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
# The Original Code is Copyright (C) 2022 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(NOM_PROJET module_c_geometrie_3d)

find_package(Eigen3 REQUIRED)
find_package(CGAL REQUIRED)

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../jorjala/
        /home/kevin/src/blender/lib/linux_centos7_x86_64/boost/include
        ${EIGEN3_INCLUDE_DIR}
        ${CGAL_INCLUDE_DIRS}
)

set(BIBLIOTHEQUES
    dls::outils
    dls::structures
    dls::memoire
    kuri::structures
)

set(SOURCES_BOOLEEN
    booleen/boolean_operations.hpp
    booleen/boolops_cpolyhedron_builder.hpp
    booleen/boolops_definitions.hpp
    booleen/boolops_enriched_polyhedron.hpp
    booleen/boolops_polyhedra.hpp
    booleen/boolops_properties.h
    booleen/boolops_triangulation.hpp

    booleen/Geometry_traits.h
    booleen/properties.h

    booleen/Geometry_traits_cgal_polyhedron_3.h
    booleen/properties_polyhedron_3.h
    booleen/Wrappings_cgal_polyhedron_3.h
    booleen/Geometry_traits_cartesian.h
    booleen/Geometry_traits_cgal_exact_predicates_inexact_constructions_kernel.h
    booleen/Geometry_traits_operators.h
)

set(SOURCES_BOOLEEN_BLENDER
    booleen_blender/BLI_allocator.hh
    booleen_blender/BLI_array.hh
    booleen_blender/BLI_assert.h
    booleen_blender/BLI_compiler_attrs.h
    booleen_blender/BLI_compiler_compat.h
    booleen_blender/BLI_compiler_typecheck.h
    booleen_blender/BLI_delaunay_2d.h
    booleen_blender/BLI_function_ref.hh
    booleen_blender/BLI_hash.h
    booleen_blender/BLI_hash.hh
    booleen_blender/BLI_hash_tables.hh
    booleen_blender/BLI_index_range.hh
    booleen_blender/BLI_kdopbvh.h
    booleen_blender/BLI_listbase.h
    booleen_blender/BLI_listbase_wrapper.hh
    booleen_blender/BLI_map.hh
    booleen_blender/BLI_map_slots.hh
    booleen_blender/BLI_math.h
    booleen_blender/BLI_math_base.h
    booleen_blender/BLI_math_base.hh
    booleen_blender/BLI_math_base_safe.h
    booleen_blender/BLI_math_boolean.hh
    booleen_blender/BLI_math_color.h
    booleen_blender/BLI_math_geom.h
    booleen_blender/BLI_math_inline.h
    booleen_blender/BLI_math_interp.h
    booleen_blender/BLI_math_matrix.h
    booleen_blender/BLI_math_mpq.hh
    booleen_blender/BLI_math_rotation.h
    booleen_blender/BLI_math_solvers.h
    booleen_blender/BLI_math_statistics.h
    booleen_blender/BLI_math_time.h
    booleen_blender/BLI_math_vec_mpq_types.hh
    booleen_blender/BLI_math_vec_types.hh
    booleen_blender/BLI_math_vector.h
    booleen_blender/BLI_math_vector.hh
    booleen_blender/BLI_memory_utils.hh
    booleen_blender/BLI_mesh_boolean.hh
    booleen_blender/BLI_mesh_intersect.hh
    booleen_blender/BLI_probing_strategies.hh
    booleen_blender/BLI_set.hh
    booleen_blender/BLI_set_slots.hh
    booleen_blender/BLI_simd.h
    booleen_blender/BLI_span.hh
    booleen_blender/BLI_stack.hh
    booleen_blender/BLI_string.h
    booleen_blender/BLI_string_ref.hh
    booleen_blender/BLI_sys_types.h
    booleen_blender/BLI_task.hh
    booleen_blender/BLI_utildefines.h
    booleen_blender/BLI_utildefines_variadic.h
    booleen_blender/BLI_utility_mixins.hh
    booleen_blender/BLI_vector.hh
    booleen_blender/BLI_vector_set.hh
    booleen_blender/BLI_vector_set_slots.hh

    booleen_blender/MEM_guardedalloc.h

    booleen_blender/DNA_listBase.h
    booleen_blender/DNA_vec_types.h

    booleen_blender/PIL_time.h

#    booleen_blender/math_base_inline.c
#    booleen_blender/math_base_safe_inline.c
#    booleen_blender/math_color_inline.c
#    booleen_blender/math_geom_inline.c
#    booleen_blender/math_vector_inline.c

    booleen_blender/mesh_boolean.cc
)

set(SOURCES_VORONOI
    voro/cell.cc
    voro/c_loops.cc
    voro/cmd_line.cc
    voro/common.cc
    voro/container.cc
    voro/container_prd.cc
    voro/pre_container.cc
    voro/unitcell.cc
    voro/v_base.cc
    voro/v_compute.cc
    voro/wall.cc

    voro/cell.hh
    voro/c_loops.hh
    voro/common.hh
    voro/config.hh
    voro/container.hh
    voro/container_prd.hh
    voro/pre_container.hh
    voro/rad_option.hh
    voro/unitcell.hh
    voro/v_base.hh
    voro/v_compute.hh
    voro/voro++.hh
    voro/wall.hh
    voro/worklist.hh
)

set(SOURCES
    acceleration.cc
    acceleration.hh
    creation.cc
    creation.h
    import_objet.cc
    import_objet.h
    ipa.cc
    ipa.h
    outils.cc
    outils.hh
    particules.cc
    particules.hh

    booleen_maillage.cc
    booleen_maillage.hh

    fracture.cc
    fracture.hh

    triangulation.cc
    triangulation.hh
    triangulation_delaunay.hh

    terrains.cc
    terrains.hh

    ${SOURCES_BOOLEEN}
    # ${SOURCES_BOOLEEN_BLENDER}
    ${SOURCES_VORONOI}
    ${CMAKE_CURRENT_SOURCE_DIR}/../InterfaceCKuri/contexte_kuri.hh
)

compile_module_c(${NOM_PROJET} geometrie3d ipa.h Géométrie3D "${SOURCES}" "${INCLUSIONS}" "${BIBLIOTHEQUES}")

desactive_drapeaux_compilation_c(${NOM_PROJET}_statique)
desactive_drapeaux_compilation_c(${NOM_PROJET}_dynamique)
