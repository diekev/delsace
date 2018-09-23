# - Find OpenSUBDIV library
# Find the native OpenSUBDIV includes and library
# This module defines
#  OPENSUBDIV_INCLUDE_DIRS, where to find ImfXdr.h, etc. Set when
#                        OPENSUBDIV_INCLUDE_DIR is found.
#  OPENSUBDIV_LIBRARIES, libraries to link against to use OpenSUBDIV.
#  OPENSUBDIV_ROOT_DIR, The base directory to search for OpenSUBDIV.
#                    This can also be an environment variable.
#  OPENSUBDIV_FOUND, If false, do not try to use OpenSUBDIV.
#
# For individual library access these advanced settings are available
#  OPENSUBDIV_HALF_LIBRARY, Path to Half library
#  OPENSUBDIV_IEX_LIBRARY, Path to Half library
#  OPENSUBDIV_ILMIMF_LIBRARY, Path to Ilmimf library
#  OPENSUBDIV_ILMTHREAD_LIBRARY, Path to IlmThread library
#  OPENSUBDIV_IMATH_LIBRARY, Path to Imath library
#
# also defined, but not for general use are
#  OPENSUBDIV_LIBRARY, where to find the OpenSUBDIV library.

#=============================================================================
# Copyright 2011 Blender Foundation.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

# If OPENSUBDIV_ROOT_DIR was defined in the environment, use it.
IF(NOT OPENSUBDIV_ROOT_DIR AND NOT $ENV{OPENSUBDIV_ROOT_DIR} STREQUAL "")
  SET(OPENSUBDIV_ROOT_DIR $ENV{OPENSUBDIV_ROOT_DIR})
ENDIF()

SET(_opensubdiv_FIND_COMPONENTS
  osdCPU
  osdGPU
)

SET(_opensubdiv_SEARCH_DIRS
  ${OPENSUBDIV_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/lib/opensubdiv
)

FIND_PATH(OPENSUBDIV_INCLUDE_DIR
  NAMES
    opensubdiv/version.h
  HINTS
    ${_opensubdiv_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

SET("OPENSUBDIV_VERSION" ${_opensubdiv_libs_ver_init} CACHE STRING "Version of OpenSUBDIV lib")
UNSET(_opensubdiv_libs_ver_init)

SET(_opensubdiv_LIBRARIES)
FOREACH(COMPONENT ${_opensubdiv_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)

  FIND_LIBRARY(OPENSUBDIV_${UPPERCOMPONENT}_LIBRARY
    NAMES
      ${COMPONENT}
    HINTS
      ${_opensubdiv_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib
    )
  LIST(APPEND _opensubdiv_LIBRARIES "${OPENSUBDIV_${UPPERCOMPONENT}_LIBRARY}")
ENDFOREACH()

UNSET(_opensubdiv_libs_ver)

# handle the QUIETLY and REQUIRED arguments and set OPENSUBDIV_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenSUBDIV  DEFAULT_MSG
    _opensubdiv_LIBRARIES OPENSUBDIV_INCLUDE_DIR)

IF(OPENSUBDIV_FOUND)
  SET(BIBLIOTHEQUES_OPENSUBDIV ${_opensubdiv_LIBRARIES})
  SET(INCLUSIONS_OPENSUBDIV ${OPENSUBDIV_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(
  OPENSUBDIV_INCLUDE_DIR
  OPENSUBDIV_VERSION
)
FOREACH(COMPONENT ${_opensubdiv_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)
  MARK_AS_ADVANCED(OPENSUBDIV_${UPPERCOMPONENT}_LIBRARY)
ENDFOREACH()

UNSET(COMPONENT)
UNSET(UPPERCOMPONENT)
UNSET(_opensubdiv_FIND_COMPONENTS)
UNSET(_opensubdiv_LIBRARIES)
UNSET(_opensubdiv_SEARCH_DIRS)
