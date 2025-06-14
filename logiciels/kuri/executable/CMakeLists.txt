# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2018-2022 Kévin Dietrich.


set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

################################################################################

set(NOM_PROJET kuri)

set(BIBLIOTHEQUES
    kuri_compilation
    kuri_parsage
    kuri_representation_intermediaire
)

add_executable(kuri
    lanceur.cc
)

target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

install(
    TARGETS ${NOM_PROJET} RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

install(FILES fichiers/execution_kuri.S DESTINATION ${RACINE_INSTALLATION}/fichiers)
install(FILES fichiers/point_d_entree.c DESTINATION ${RACINE_INSTALLATION}/fichiers)
install(FILES fichiers/point_d_entree_dynamique.c DESTINATION ${RACINE_INSTALLATION}/fichiers)

################################################################################

add_executable(kuri_format
    kuri_format.cc
)

target_include_directories(kuri_format PUBLIC "${INCLUSIONS}")

target_link_libraries(kuri_format "${BIBLIOTHEQUES}")

install(
    TARGETS kuri_format RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

if(AVEC_KURI_REMPLACE)
    add_executable(kuri_remplace
        kuri_remplace.cc
    )

    target_include_directories(kuri_remplace PUBLIC "${INCLUSIONS}")

    target_link_libraries(kuri_remplace "${BIBLIOTHEQUES}" bib_json dls::outils)

    install(
        TARGETS kuri_remplace RUNTIME DESTINATION ${RACINE_INSTALLATION}
    )
endif()

################################################################################

if(AVEC_DEMON_KURI)
    add_executable(demon_kuri
        demon.cc
    )

    target_include_directories(demon_kuri PUBLIC "${INCLUSIONS}")

    target_link_libraries(demon_kuri "${BIBLIOTHEQUES}")

    install(
        TARGETS demon_kuri RUNTIME DESTINATION ${RACINE_INSTALLATION}
    )
endif()

################################################################################

add_executable(syntaxage_ri
    syntaxage_ri.cc
)

target_include_directories(syntaxage_ri PUBLIC "${INCLUSIONS}")

target_link_libraries(syntaxage_ri "${BIBLIOTHEQUES}")

install(
    TARGETS syntaxage_ri RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

add_executable(test_ri
    test_ri.cc
)

target_include_directories(test_ri PUBLIC "${INCLUSIONS}")

target_link_libraries(test_ri "${BIBLIOTHEQUES}")

install(
    TARGETS test_ri RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

add_executable(test_fsau
    test_fsau.cc
)

target_include_directories(test_fsau PUBLIC "${INCLUSIONS}")

target_link_libraries(test_fsau "${BIBLIOTHEQUES}")

install(
    TARGETS test_fsau RUNTIME DESTINATION ${RACINE_INSTALLATION}
)

################################################################################

if(AVEC_KURI_PARCER AND AVEC_LLVM)
    set(NOM_PROJET parcer)

    set(INCLUSIONS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../
        ${INCLUSIONS_DELSACE_CHRONO}
        ${CLANG_INCLUDE_DIR}
    )

    set(BIBLIOTHEQUES
        ${BIBLIOTHEQUE_CXX_FILESYSTEM}
        bib_outils
        bib_json
        kuri_structures
        ${CLANG_LIBRARIES}
        ${LLVM_LIBRARY}
    )

    add_executable(${NOM_PROJET}
        parcer.cc
    )

    get_target_property(EXTLIB_COMPILE_FLAGS ${NOM_PROJET} COMPILE_OPTIONS)
    list(REMOVE_ITEM EXTLIB_COMPILE_FLAGS -Wundef)
    list(APPEND EXTLIB_COMPILE_FLAGS -Wno-comment)
    set_target_properties(${NOM_PROJET} PROPERTIES COMPILE_OPTIONS "${EXTLIB_COMPILE_FLAGS}")

    # Force l'utilisation de C++ 17 car LLVM utilise des fonctions du
    # langage qui sont déclarées comme malformées en C++ 20.
    set_property(TARGET ${NOM_PROJET} PROPERTY CXX_STANDARD 17)

    target_include_directories(${NOM_PROJET} PUBLIC "${INCLUSIONS}")

    target_link_libraries(${NOM_PROJET} "${BIBLIOTHEQUES}")

    install(
        TARGETS ${NOM_PROJET} RUNTIME DESTINATION .
    )
endif()
