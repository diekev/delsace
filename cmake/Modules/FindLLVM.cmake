# - Find LLVM library
# Find the native LLVM includes and library
# This module defines
#  LLVM_INCLUDE_DIRS, where to find llvm.h, Set when
#                    LLVM is found.
#  LLVM_LIBRARIES, libraries to link against to use LLVM.
#  LLVM_ROOT_DIR, The base directory to search for LLVM.
#                This can also be an environment variable.
#  LLVM_FOUND, If false, do not try to use LLVM.
#
# also defined, but not for general use are
#  LLVM_LIBRARY, where to find the LLVM library.

# If LLVM_ROOT_DIR was defined in the environment, use it.

set(LLVM_VERSION 12)

if(LLVM_ROOT_DIR)
    if(DEFINED LLVM_VERSION)
        find_program(LLVM_CONFIG llvm-config-${LLVM_VERSION} HINTS ${LLVM_ROOT_DIR}/bin NO_CMAKE_PATH)
    endif()
    if(NOT LLVM_CONFIG)
        find_program(LLVM_CONFIG llvm-config HINTS ${LLVM_ROOT_DIR}/bin NO_CMAKE_PATH)
    endif()
else()
    if(DEFINED LLVM_VERSION)
        message(running llvm-config-${LLVM_VERSION})
        find_program(LLVM_CONFIG llvm-config-${LLVM_VERSION})
    endif()
    if(NOT LLVM_CONFIG)
        find_program(LLVM_CONFIG llvm-config)
    endif()
endif()

if(NOT DEFINED LLVM_VERSION)
    execute_process(COMMAND ${LLVM_CONFIG} --version
        OUTPUT_VARIABLE LLVM_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_VERSION ${LLVM_VERSION} CACHE STRING "Version of LLVM to use")
endif()
if(NOT LLVM_ROOT_DIR)
    execute_process(COMMAND ${LLVM_CONFIG} --prefix
        OUTPUT_VARIABLE LLVM_ROOT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_ROOT_DIR ${LLVM_ROOT_DIR} CACHE PATH "Path to the LLVM installation")
endif()
if(NOT LLVM_LIBPATH)
    execute_process(COMMAND ${LLVM_CONFIG} --libdir
        OUTPUT_VARIABLE LLVM_LIBPATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_LIBPATH ${LLVM_LIBPATH} CACHE PATH "Path to the LLVM library path")
    mark_as_advanced(LLVM_LIBPATH)
endif()

if(LLVM_STATIC)
    find_library(LLVM_LIBRARY
        NAMES LLVMDemangle # first of a whole bunch of libs to get
        PATHS ${LLVM_LIBPATH})
else()
    find_library(LLVM_LIBRARY
        NAMES
        LLVM-${LLVM_VERSION}
        LLVMDemangle  # check for the static library as a fall-back
        PATHS ${LLVM_LIBPATH})
endif()

find_path(LLVM_INCLUDE_DIR
    NAMES
        llvm/InitializePasses.h
    HINTS
        ${LLVM_ROOT_DIR}
    PATH_SUFFIXES
        include
)

find_path(CLANG_INCLUDE_DIR
    NAMES
        clang/AST/Expr.h
    HINTS
        ${LLVM_ROOT_DIR}
    PATH_SUFFIXES
        include
)

if(LLVM_LIBRARY AND LLVM_ROOT_DIR AND LLVM_LIBPATH)
    if(LLVM_STATIC)
        # if static LLVM libraries were requested, use llvm-config to generate
        # the list of what libraries we need, and substitute that in the right
        # way for LLVM_LIBRARY.
        execute_process(COMMAND ${LLVM_CONFIG} --libfiles
            OUTPUT_VARIABLE LLVM_LIBRARY
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REPLACE " " ";" LLVM_LIBRARY "${LLVM_LIBRARY}")
    endif()
endif()

if(LLVM_LIBPATH)
    set(CLANG_LIBS
        clangTooling
        clangFrontend
        clangFrontendTool
        clangDriver
        clangSerialization
        clangCodeGen
        clangParse
        clangSema
        clangStaticAnalyzerFrontend
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangAnalysis
        clangARCMigrate
        clangRewrite
        clangRewriteFrontend
        clangEdit
        clangAST
        clangLex
        clangBasic
        clang
    )

    set(CLANG_LIBRARIES)

    foreach (__clang_lib__ ${CLANG_LIBS})
        find_library(__clang_library__${__clang_lib__} NAMES ${__clang_lib__} PATHS ${LLVM_LIBPATH} NO_CACHE)
        list(APPEND CLANG_LIBRARIES ${__clang_library__${__clang_lib__}})
    endforeach()
endif()

# handle the QUIETLY and REQUIRED arguments and set LLVM_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM DEFAULT_MSG
    LLVM_LIBRARY  LLVM_INCLUDE_DIR CLANG_INCLUDE_DIR CLANG_LIBRARIES)

mark_as_advanced(
    LLVM_LIBRARY
)
