# - Find Ego library
# Find the native MATH includes and library
# This module defines
#  INCLUSIONS_MATH, where to find math.h, Set when
#                    MATH is found.
#  BIBLIOTHEQUES_MATH, libraries to link against to use MATH.
#  MATH_ROOT_DIR, The base directory to search for MATH.
#                This can also be an environment variable.
#  MATH_FOUND, If false, do not try to use MATH.
#
# also defined, but not for general use are
#  MATH_LIBRARY, where to find the MATH library.

# If MATH_ROOT_DIR was defined in the environment, use it.
if(NOT MATH_ROOT_DIR AND NOT $ENV{MATH_ROOT_DIR} STREQUAL "")
	set(MATH_ROOT_DIR $ENV{MATH_ROOT_DIR})
endif()

set(_math_SEARCH_DIRS
	${MATH_ROOT_DIR}
	/opt/lib/numero7
)

find_path(MATH_INCLUDE_DIR
	NAMES
	    math/vec3.h
	HINTS
	    ${_math_SEARCH_DIRS}
	PATH_SUFFIXES
	    include/numero7
)

find_library(MATH_LIBRARY
	NAMES
	    math
	HINTS
	    ${_math_SEARCH_DIRS}
	PATH_SUFFIXES
	    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set MATH_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATH DEFAULT_MSG
	    MATH_LIBRARY MATH_INCLUDE_DIR)

if(MATH_FOUND)
	set(BIBLIOTHEQUES_MATH ${MATH_LIBRARY})
	set(INCLUSIONS_MATH ${MATH_INCLUDE_DIR})
else()
	set(MATH_MATH_FOUND FALSE)
endif()

mark_as_advanced(
	MATH_INCLUDE_DIR
	MATH_LIBRARY
)

unset(_math_SEARCH_DIRS)
