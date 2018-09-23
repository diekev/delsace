# - Find Ego library
# Find the native IMAGE includes and library
# This module defines
#  IMAGE_INCLUDE_DIRS, where to find image.h, Set when
#                    IMAGE is found.
#  IMAGE_LIBRARIES, libraries to link against to use IMAGE.
#  IMAGE_ROOT_DIR, The base directory to search for IMAGE.
#                This can also be an environment variable.
#  IMAGE_FOUND, If false, do not try to use IMAGE.
#
# also defined, but not for general use are
#  IMAGE_LIBRARY, where to find the IMAGE library.

# If IMAGE_ROOT_DIR was defined in the environment, use it.
if(NOT IMAGE_ROOT_DIR AND NOT $ENV{IMAGE_ROOT_DIR} STREQUAL "")
	set(IMAGE_ROOT_DIR $ENV{IMAGE_ROOT_DIR})
endif()

set(_image_SEARCH_DIRS
	${IMAGE_ROOT_DIR}
	/opt/lib/numero7
)

find_path(IMAGE_INCLUDE_DIR
	NAMES
	    image/pixel.h
	HINTS
	    ${_image_SEARCH_DIRS}
	PATH_SUFFIXES
	    include/numero7
)

find_library(IMAGE_LIBRARY
	NAMES
	    image
	HINTS
	    ${_image_SEARCH_DIRS}
	PATH_SUFFIXES
	    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set IMAGE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IMAGE DEFAULT_MSG
	    IMAGE_LIBRARY IMAGE_INCLUDE_DIR)

if(IMAGE_FOUND)
	set(BIBLIOTHEQUES_IMAGE ${IMAGE_LIBRARY})
	set(INCLUSIONS_IMAGE ${IMAGE_INCLUDE_DIR})
else()
	set(IMAGE_FOUND FALSE)
endif()

mark_as_advanced(
	IMAGE_INCLUDE_DIR
	IMAGE_LIBRARY
)

unset(_image_SEARCH_DIRS)
