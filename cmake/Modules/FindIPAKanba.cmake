# - Trouve le paquet d'IPA de Kanba.

if(NOT IPAKanba_ROOT AND DEFINED ENV{RACINE_IPA_KANBA})
    set(IPAKanba_ROOT $ENV{RACINE_IPA_KANBA})
endif()

if(IPAKanba_ROOT)
    find_library(
        KANBA_LIB
        NAMES "libkanba.so"
        PATHS ${IPAKanba_ROOT}
        PATH_SUFFIXES "ipa"
        NO_DEFAULT_PATH
    )

    find_path(
        KANBA_INCLUDE_DIRS
        NAMES "kanba.hh"
        PATHS ${IPAKanba_ROOT}
        PATH_SUFFIXES "ipa"
        NO_DEFAULT_PATH
    )
endif(IPAKanba_ROOT)


if(KANBA_LIB)
    set(KANBA_LIB_FOUND TRUE)
    set(KANBA_LIBRARIES ${KANBA_LIBRARIES} ${KANBA_LIB})
    add_library(Kanba::IPA INTERFACE IMPORTED)
    set_target_properties(Kanba::IPA
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${KANBA_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${KANBA_LIB}"
        )
else()
    set(KANBA_LIB_FOUND FALSE)
endif()


find_package_handle_standard_args(IPAKanba
    REQUIRED_VARS KANBA_LIBRARIES KANBA_INCLUDE_DIRS
    HANDLE_COMPONENTS
    )

mark_as_advanced(
    KANBA_INCLUDE_DIRS
    KANBA_LIBRARIES
    )
