# - Find LibFFI library
# Find the native LIBFFI includes and library
# This module defines
#  LIBFFI_INCLUDE_DIRS, where to find libffi.h, Set when
#                    LIBFFI is found.
#  LIBFFI_LIBRARIES, libraries to link against to use LIBFFI.
#  LIBFFI_ROOT_DIR, The base directory to search for LIBFFI.
#                This can also be an environment variable.
#  LIBFFI_FOUND, If false, do not try to use LIBFFI.
#
# also defined, but not for general use are
#  LIBFFI_LIBRARY, where to find the LibFFI library.

# If LIBFFI_ROOT_DIR was defined in the environment, use it.
if(NOT LIBFFI_ROOT_DIR AND NOT $ENV{LIBFFI_ROOT_DIR} STREQUAL "")
	set(LIBFFI_ROOT_DIR $ENV{LIBFFI_ROOT_DIR})
endif()

set(_libffi_SEARCH_DIRS
	${LIBFFI_ROOT_DIR}
	/opt/lib/libffi
)

find_path(LIBFFI_INCLUDE_DIR
	NAMES
	    ffi.h ffitarget.h
	HINTS
	    ${_libffi_SEARCH_DIRS}
	PATH_SULIBFFIXES
		include
)

find_library(LIBFFI_LIBRARY
	NAMES
	    ffi
	HINTS
	    ${_libffi_SEARCH_DIRS}
	PATH_SULIBFFIXES
		lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set LIBFFI_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBFFI DEFAULT_MSG
	LIBFFI_LIBRARY LIBFFI_INCLUDE_DIR)

if(LIBFFI_FOUND)
	set(BIBLIOTHEQUES_LIBFFI ${LIBFFI_LIBRARY})
	set(INCLUSIONS_LIBFFI ${LIBFFI_INCLUDE_DIR} ${LIBFFI_INCLUDE_DIR}/../)
else()
	set(LIBFFI_FOUND FALSE)
endif()

mark_as_advanced(
	LIBFFI_INCLUDE_DIR
	LIBFFI_LIBRARY
)

unset(_libffi_SEARCH_DIRS)
