# - Find Ptex library
# Find the native PTEX includes and library
# This module defines
#  PTEX_INCLUDE_DIRS, where to find ptex.h, Set when
#                    PTEX is found.
#  PTEX_LIBRARIES, libraries to link against to use PTEX.
#  PTEX_ROOT_DIR, The base directory to search for PTEX.
#                This can also be an environment variable.
#  PTEX_FOUND, If false, do not try to use PTEX.
#
# also defined, but not for general use are
#  PTEX_LIBRARY, where to find the Ptex library.

# If PTEX_ROOT_DIR was defined in the environment, use it.
if(NOT PTEX_ROOT_DIR AND NOT $ENV{PTEX_ROOT_DIR} STREQUAL "")
	set(PTEX_ROOT_DIR $ENV{PTEX_ROOT_DIR})
endif()

set(_ptex_SEARCH_DIRS
	${PTEX_ROOT_DIR}
	/opt/lib/ptex
)

find_path(PTEX_INCLUDE_DIR
	NAMES
	    Ptexture.h
	HINTS
	    ${_ptex_SEARCH_DIRS}
	PATH_SUFFIXES
		include
)

find_library(PTEX_LIBRARY
	NAMES
	    Ptex
	HINTS
	    ${_ptex_SEARCH_DIRS}
	PATH_SUFFIXES
		lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set PTEX_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PTEX DEFAULT_MSG
	PTEX_LIBRARY PTEX_INCLUDE_DIR)

if(PTEX_FOUND)
	set(PTEX_LIBRARIES ${PTEX_LIBRARY})
	set(PTEX_INCLUDE_DIRS ${PTEX_INCLUDE_DIR} ${PTEX_INCLUDE_DIR}/../)
else()
	set(PTEX_FOUND FALSE)
endif()

mark_as_advanced(
	PTEX_INCLUDE_DIR
	PTEX_LIBRARY
)

unset(_ptex_SEARCH_DIRS)
