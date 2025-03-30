# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2021 Kévin Dietrich.

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

################################################################################

set(NOM_BIBLIOTHEQUE kuri_outils_adn)

add_library(${NOM_BIBLIOTHEQUE} STATIC outils_independants_des_lexemes.cc outils_independants_des_lexemes.hh)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")

################################################################################

set(NOM_BIBLIOTHEQUE kuri_outils_dependants_sur_lexemes_adn)

add_library(${NOM_BIBLIOTHEQUE} STATIC outils_dependants_sur_lexemes.cc outils_dependants_sur_lexemes.hh)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")
add_dependencies(${NOM_BIBLIOTHEQUE} kuri_parsage)

################################################################################

set(NOM_BIBLIOTHEQUE kuri_adn)

add_library(${NOM_BIBLIOTHEQUE} STATIC adn.cc adn.hh)
target_include_directories(${NOM_BIBLIOTHEQUE} PUBLIC "${INCLUSIONS}")
target_link_libraries(${NOM_BIBLIOTHEQUE} kuri_parsage kuri_outils_adn)
add_dependencies(${NOM_BIBLIOTHEQUE} kuri_parsage)

################################################################################

set(NOM_EXECUTABLE genere_lexemes)

set(BIBLIOTHEQUES
    kuri_outils_adn
    kuri_structures
    dls::langage
)

add_executable(${NOM_EXECUTABLE} genere_lexemes.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_ipa)

set(INCLUSIONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

set(BIBLIOTHEQUES
    kuri_adn
    kuri_outils_adn
    kuri_outils_dependants_sur_lexemes_adn
    kuri_parsage
    kuri_structures
    bib_structures
    bib_moultfilage
    langage
)

add_executable(${NOM_EXECUTABLE} genere_ipa.cc)

target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

add_dependencies(${NOM_EXECUTABLE} kuri_parsage)

################################################################################

set(NOM_EXECUTABLE genere_options)

set(BIBLIOTHEQUES
    kuri_adn
    kuri_outils_adn
    kuri_outils_dependants_sur_lexemes_adn
    kuri_parsage
    kuri_structures
    bib_structures
    bib_moultfilage
    langage
)

add_executable(${NOM_EXECUTABLE} genere_options.cc)

target_compile_definitions(genere_options PUBLIC ACCES_LEXEMES_EST_POSSIBLE)
target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_message)

set(BIBLIOTHEQUES
    kuri_adn
    kuri_outils_adn
    kuri_outils_dependants_sur_lexemes_adn
    kuri_parsage
    kuri_structures
    bib_structures
    bib_moultfilage
    langage
)

add_executable(${NOM_EXECUTABLE} genere_message.cc)

target_compile_definitions(genere_message PUBLIC ACCES_LEXEMES_EST_POSSIBLE)
target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_noeuds)

set(BIBLIOTHEQUES
    kuri_adn
    kuri_outils_adn
    kuri_outils_dependants_sur_lexemes_adn
    kuri_parsage
    kuri_structures
    bib_structures
    bib_moultfilage
    langage
)

add_executable(${NOM_EXECUTABLE} genere_noeuds.cc)

target_compile_definitions(genere_noeuds PUBLIC ACCES_LEXEMES_EST_POSSIBLE)
target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")

################################################################################

set(NOM_EXECUTABLE genere_intrinseques)

set(BIBLIOTHEQUES
    kuri_adn
    kuri_outils_adn
    kuri_outils_dependants_sur_lexemes_adn
    kuri_parsage
    kuri_structures
    bib_structures
    bib_moultfilage
    langage
)

add_executable(${NOM_EXECUTABLE} genere_intrinseques.cc)

target_compile_definitions(genere_noeuds PUBLIC ACCES_LEXEMES_EST_POSSIBLE)
target_include_directories(${NOM_EXECUTABLE} PUBLIC "${INCLUSIONS}")

target_link_libraries(${NOM_EXECUTABLE} "${BIBLIOTHEQUES}")
