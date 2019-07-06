project(Patate-common)
cmake_minimum_required(VERSION 2.8)


FILE ( GLOB COMMON_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/defines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/surface_mesh/*.h
)

FILE ( GLOB COMMON_IMPL
  ${CMAKE_CURRENT_SOURCE_DIR}/surface_mesh/*.hpp
)

set (DEPS ${COMMON_HEADERS} ${COMMON_IMPL})

find_package(OpenGL)
if(OPENGL_FOUND)
    message("Including OpenGL files in Patate common library")
    FILE ( GLOB COMMON_GL_HEADERS
      ${CMAKE_CURRENT_SOURCE_DIR}/gl_utils/*.h
    )

    FILE ( GLOB COMMON_GL_IMPL
      ${CMAKE_CURRENT_SOURCE_DIR}/gl_utils/*.hpp
    )

    set(DEPS ${DEPS} ${COMMON_GL_IMPL} ${COMMON_GL_HEADERS})
endif()

add_library (patate_common OBJECT ${DEPS})
set_target_properties(patate_common PROPERTIES LINKER_LANGUAGE C++)
