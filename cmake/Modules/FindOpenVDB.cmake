# Trouve la bibliothèque OpenVDB

# If OPENVDB_ROOT_DIR was defined in the environment, use it.
IF(NOT OPENVDB_ROOT_DIR AND NOT $ENV{OPENVDB_ROOT_DIR} STREQUAL "")
  SET(OPENVDB_ROOT_DIR $ENV{OPENVDB_ROOT_DIR})
ENDIF()


set(_openvdb_SEARCH_DIRS
  ${OPENVDB_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/lib/openvdb
)

find_path(OPENVDB_INCLUDE_DIR
  NAMES
    openvdb/openvdb.h
  HINTS
    ${_openvdb_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

find_library(OPENVDB_LIBRARY
	NAMES
	    openvdb
	HINTS
	    ${_openvdb_SEARCH_DIRS}
	PATH_SUFFIXES
	    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set OPENVDB_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVDB  DEFAULT_MSG
    OPENVDB_LIBRARY OPENVDB_INCLUDE_DIR)

IF(OPENVDB_FOUND)
  SET(BIBLIOTHEQUES_OPENVDB ${OPENVDB_LIBRARY})
  SET(INCLUSIONS_OPENVDB ${OPENVDB_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(
  OPENVDB_INCLUDE_DIR
  OPENVDB_VERSION
)

