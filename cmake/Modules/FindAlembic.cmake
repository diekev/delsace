# - Find Alembic library
# Find the native ALEMBIC includes and library
# This module defines
#  ALEMBIC_INCLUDE_DIRS, where to find alembic.h, Set when
#                    ALEMBIC is found.
#  ALEMBIC_LIBRARIES, libraries to link against to use ALEMBIC.
#  ALEMBIC_ROOT_DIR, The base directory to search for ALEMBIC.
#                This can also be an environment variable.
#  ALEMBIC_FOUND, If false, do not try to use ALEMBIC.
#
# also defined, but not for general use are
#  ALEMBIC_LIBRARY, where to find the Alembic library.

# If ALEMBIC_ROOT_DIR was defined in the environment, use it.
if(NOT ALEMBIC_ROOT_DIR AND NOT $ENV{ALEMBIC_ROOT_DIR} STREQUAL "")
	set(ALEMBIC_ROOT_DIR $ENV{ALEMBIC_ROOT_DIR})
endif()

set(_alembic_SEARCH_DIRS
	${ALEMBIC_ROOT_DIR}
	/opt/lib/alembic
)

find_path(ALEMBIC_INCLUDE_DIR
	NAMES
	    Alembic/Abc/All.h
	HINTS
	    ${_alembic_SEARCH_DIRS}
	PATH_SUFFIXES
		include
)

find_library(ALEMBIC_LIBRARY
	NAMES
	    Alembic
	HINTS
	    ${_alembic_SEARCH_DIRS}
	PATH_SUFFIXES
		lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set ALEMBIC_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ALEMBIC DEFAULT_MSG
	ALEMBIC_LIBRARY ALEMBIC_INCLUDE_DIR)

if(ALEMBIC_FOUND)
	set(BIBLIOTHEQUES_ALEMBIC ${ALEMBIC_LIBRARY})
	set(INCLUSIONS_ALEMBIC ${ALEMBIC_INCLUDE_DIR} ${ALEMBIC_INCLUDE_DIR}/../)
else()
	set(ALEMBIC_FOUND FALSE)
endif()

mark_as_advanced(
	ALEMBIC_INCLUDE_DIR
	ALEMBIC_LIBRARY
)

unset(_alembic_SEARCH_DIRS)
