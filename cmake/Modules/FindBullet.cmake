# Trouve la bibliothèque Bullet Physics

# Utilise BULLET_ROOT_DIR si défini dans l'environnement.
IF(NOT BULLET_ROOT_DIR AND NOT $ENV{BULLET_ROOT_DIR} STREQUAL "")
  SET(BULLET_ROOT_DIR $ENV{BULLET_ROOT_DIR})
ENDIF()

SET(_bullet_FIND_COMPONENTS
    Bullet2FileLoader
    # Bullet 3
    Bullet3Collision
    Bullet3Common
    Bullet3Dynamics
    Bullet3Geometry
    Bullet3OpenCL_clew
    #Bullet
    BulletCollision
    BulletDynamics
    BulletExampleBrowserLib
    BulletFileLoader
    BulletInverseDynamics
    BulletInverseDynamicsUtils
    BulletRobotics
    BulletSoftBody
    BulletWorldImporter
    BulletXmlWorldImporter
    BussIK
    clsocket
    ConvexDecomposition
    GIMPACTUtils
    HACD
    LinearMath
)

SET(_bullet_SEARCH_DIRS
  ${BULLET_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/lib/bullet
)

FIND_PATH(BULLET_INCLUDE_DIR
  NAMES
    bullet/btBulletCollisionCommon.h
  HINTS
    ${_bullet_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

SET(_bullet_LIBRARIES)
FOREACH(COMPONENT ${_bullet_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)

  FIND_LIBRARY(BULLET_${UPPERCOMPONENT}_LIBRARY
    NAMES
      ${COMPONENT}-${_bullet_libs_ver} ${COMPONENT}
    HINTS
      ${_bullet_SEARCH_DIRS}
    PATH_SUFFIXES
      lib64 lib
    )
  LIST(APPEND _bullet_LIBRARIES "${BULLET_${UPPERCOMPONENT}_LIBRARY}")
ENDFOREACH()

UNSET(_bullet_libs_ver)

# Gestion des arguments QUIETLY et REQUIRED et met BULLET_FOUND = TRUE 
# si toutes les variables listées sont TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Bullet
    DEFAULT_MSG
    _bullet_LIBRARIES
    BULLET_INCLUDE_DIR)

IF(BULLET_FOUND)
  SET(BIBLIOTHEQUES_BULLET ${_bullet_LIBRARIES})
  SET(INCLUSIONS_BULLET ${BULLET_INCLUDE_DIR} ${BULLET_INCLUDE_DIR}/bullet/)
ENDIF()

MARK_AS_ADVANCED(
  BULLET_INCLUDE_DIR
)

FOREACH(COMPONENT ${_bullet_FIND_COMPONENTS})
  STRING(TOUPPER ${COMPONENT} UPPERCOMPONENT)
  MARK_AS_ADVANCED(BULLET_${UPPERCOMPONENT}_LIBRARY)
ENDFOREACH()

UNSET(COMPONENT)
UNSET(UPPERCOMPONENT)
UNSET(_bullet_FIND_COMPONENTS)
UNSET(_bullet_LIBRARIES)
UNSET(_bullet_SEARCH_DIRS)
