# - Find Curl library
# Find the native CURL includes and library
# This module defines
#  INCLUSIONS_CURL, where to find curl.h, Set when
#                    CURL is found.
#  BIBLIOTHEQUES_CURL, libraries to link against to use CURL.
#  CURL_ROOT_DIR, The base directory to search for CURL.
#                This can also be an environment variable.
#  CURL_FOUND, If false, do not try to use CURL.
#
# also defined, but not for general use are
#  CURL_LIBRARY, where to find the CURL library.

# If CURL_ROOT_DIR was defined in the environment, use it.
if(NOT CURL_ROOT_DIR AND NOT $ENV{CURL_ROOT_DIR} STREQUAL "")
	set(CURL_ROOT_DIR $ENV{CURL_ROOT_DIR})
endif()

set(_curl_SEARCH_DIRS
	${CURL_ROOT_DIR}
	/opt/lib/curl
)

find_path(CURL_INCLUDE_DIR
	NAMES
		curl.h
	HINTS
	    ${_curl_SEARCH_DIRS}
	PATH_SUFFIXES
		include/curl
)

find_library(CURL_LIBRARY
	NAMES
	    curl
	HINTS
	    ${_curl_SEARCH_DIRS}
	PATH_SUFFIXES
	    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG
	    CURL_LIBRARY CURL_INCLUDE_DIR)

if(CURL_FOUND)
	set(BIBLIOTHEQUES_CURL ${CURL_LIBRARY})
	set(INCLUSIONS_CURL ${CURL_INCLUDE_DIR})
else()
	set(CURL_FOUND FALSE)
endif()

mark_as_advanced(
	CURL_INCLUDE_DIR
	CURL_LIBRARY
)

unset(_curl_SEARCH_DIRS)
