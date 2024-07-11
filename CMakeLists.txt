# SPDX-License-Identifier: GPL-2.0-or-later
# The Original Code is Copyright (C) 2018 Kévin Dietrich.

# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.8)

# ------------------------------------------------------------------------------

project(delsace)

# ------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform")

# ------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE_INIT "Release")

# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)

option(AVEC_ASAN "Utilise asan lors de la compilation en mode débogage" OFF)
option(AVEC_CRESUS "Compiler Crésus" ON)
option(AVEC_DANJO "Compiler le langage de création d'interface Danjo" ON)
option(AVEC_DEPLOIE "Compiler l'outil de déploiement d'application web Déploie" ON)
option(AVEC_HIKAGE "Compiler Hikage" ON)
option(AVEC_HIMITSU "Compiler Himitsu" ON)
option(AVEC_IMAGO "Compiler Imago" ON)
option(AVEC_INFOGRAPHIE "Compiler Infographie" ON)
option(AVEC_JORJALA "Compiler le logiciel de création 3D et compositing Jorjala" ON)
option(AVEC_KANBA "Compiler le logiciel de peinture 3D Kanba" ON)
option(AVEC_KUMO "Compiler le langage de création de tables MySQL Kumo" ON)
option(AVEC_KURI "Compiler le compilateur du langage Kuri" ON)
option(AVEC_KYANBASU "Compiler Kyanbasu" ON)
option(AVEC_LLVM "Active l'utilisation de la bibliothèque LLVM" OFF)
option(AVEC_MESURE "Compiler le langage de statistiques de lignes de code Mesure" ON)
option(AVEC_SILVATHEQUE "Compiler le logiciel de création d'arbre 3D Silvathèque" ON)
option(AVEC_TESTS "Compiler les tests des bibliothèques internes" ON)
option(AVEC_VULKAN "Compiler le logiciel test de Vulkan" ON)
option(AVEC_DEMON_REACT "Compiler le démon de compilation de projets React" OFF)

option(SEULEMENT_KURI "Ne compiler que Kuri et ses dépendances" OFF)

# ------------------------------------------------------------------------------

# Évite de polluer les options pour l'instant.
set(REQUIERS_QT OFF)
set(REQUIERS_TBB ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_OPENGL OFF)
set(REQUIERS_BOOST ON)
set(REQUIERS_DOCOPT OFF)
set(REQUIERS_KELVINLET OFF)
set(REQUIERS_PATATE OFF)
set(REQUIERS_VORO OFF)
set(REQUIERS_CGAL OFF)
set(REQUIERS_IPROF OFF)

# Options pour les bibliothèques internes
set(REQUIERS_BIB_BRUIT OFF)
set(REQUIERS_BIB_CHISEI OFF)
set(REQUIERS_BIB_CHRONO ON) # Toujours ON
set(REQUIERS_BIB_COMPILER OFF)
set(REQUIERS_BIB_COMPRESSION OFF)
set(REQUIERS_BIB_EGO OFF)
set(REQUIERS_BIB_EXERCISES OFF)
set(REQUIERS_BIB_FINANCE OFF)
set(REQUIERS_BIB_FLUID_RETIMER OFF)
set(REQUIERS_BIB_FLUX OFF)
set(REQUIERS_BIB_GRAPHE OFF)
set(REQUIERS_BIB_IMAGE OFF)
set(REQUIERS_BIB_JSON OFF)
set(REQUIERS_BIB_LANGAGE OFF)
set(REQUIERS_BIB_LINGUA OFF)
set(REQUIERS_BIB_MATH OFF)
set(REQUIERS_BIB_MEMOIRE ON) # Toujours ON
set(REQUIERS_BIB_MOULTFILAGE OFF)
set(REQUIERS_BIB_NOMBRE_DECIMAUX OFF)
set(REQUIERS_BIB_OBJETS OFF)
set(REQUIERS_BIB_OPENCV OFF)
set(REQUIERS_BIB_OPENGL OFF)
set(REQUIERS_BIB_OUTILS ON) # Toujours ON
set(REQUIERS_BIB_PATRONS_CONCEPTION OFF)
set(REQUIERS_BIB_PHYS OFF)
set(REQUIERS_BIB_REFERENCE OFF)
set(REQUIERS_BIB_RUNTIME_POLYMORPHY OFF)
set(REQUIERS_BIB_STRUCTURES ON) # Toujours ON
set(REQUIERS_BIB_SYSTEME_FICHIER OFF)
set(REQUIERS_BIB_TESTS ON) # Toujours ON
set(REQUIERS_BIB_TEXTURE OFF)
set(REQUIERS_BIB_TORI OFF)
set(REQUIERS_BIB_TREES OFF)
set(REQUIERS_BIB_TYPES ON) # Requis pour les tests
set(REQUIERS_BIB_UNICODE OFF)
set(REQUIERS_BIB_VISION OFF)
set(REQUIERS_BIB_XML OFF)

if(SEULEMENT_KURI)
    set(AVEC_KURI ON)

    set(AVEC_CRESUS OFF)
    set(AVEC_DANJO OFF)
    set(AVEC_DEPLOIE OFF)
    set(AVEC_HIKAGE OFF)
    set(AVEC_HIMITSU OFF)
    set(AVEC_IMAGO OFF)
    set(AVEC_INFOGRAPHIE OFF)
    # set(AVEC_KANBA OFF)
    set(AVEC_KUMO OFF)
    set(AVEC_KYANBASU OFF)
    set(AVEC_MESURE OFF)
    # set(AVEC_JORJALA OFF)
    set(REQUIERS_QT OFF)
    set(AVEC_SILVATHEQUE OFF)
    set(AVEC_TESTS OFF)
    set(AVEC_VULKAN OFF)
    set(AVEC_DEMON_REACT OFF)

    # désactivation des bibliothèques internes non-requises
    set(REQUIERS_BIB_BRUIT OFF)
    set(REQUIERS_BIB_CHISEI OFF)
    set(REQUIERS_BIB_COMPILER OFF)
    set(REQUIERS_BIB_COMPRESSION OFF)
    set(REQUIERS_BIB_EGO OFF)
    set(REQUIERS_BIB_EXERCISES OFF)
    set(REQUIERS_BIB_FINANCE OFF)
    set(REQUIERS_BIB_FLUID_RETIMER OFF)
    set(REQUIERS_BIB_FLUX OFF)
    set(REQUIERS_BIB_GRAPHE OFF)
    set(REQUIERS_BIB_IMAGE OFF)
    set(REQUIERS_BIB_LINGUA OFF)
    set(REQUIERS_BIB_MATH OFF)
    set(REQUIERS_BIB_OBJETS OFF)
    set(REQUIERS_BIB_OPENCV OFF)
    set(REQUIERS_BIB_OPENGL OFF)
    set(REQUIERS_BIB_PATRONS_CONCEPTION OFF)
    set(REQUIERS_BIB_PHYS OFF)
    set(REQUIERS_BIB_REFERENCE OFF)
    set(REQUIERS_BIB_RUNTIME_POLYMORPHY OFF)
    set(REQUIERS_BIB_TEXTURE OFF)
    set(REQUIERS_BIB_TREES OFF)
    set(REQUIERS_BIB_TYPES OFF)
    set(REQUIERS_BIB_UNICODE OFF)
    set(REQUIERS_BIB_VISION OFF)
    set(REQUIERS_BIB_XML OFF)
endif(SEULEMENT_KURI)

macro(ajoute_sousdossier __test__ __nom__)
    if (${__test__})
        add_subdirectory(${__nom__})
    endif()
endmacro()

function(active_dependances __test__)
    if (${__test__})
        foreach(dep ${ARGN})
            set(${dep} ON PARENT_SCOPE)
        endforeach()
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Active les dépendances des programmes.

active_dependances(
    AVEC_JORJALA

    AVEC_DANJO
    #REQUIERS_BIB_BRUIT
    REQUIERS_BIB_IMAGE
    REQUIERS_CGAL
    #REQUIERS_BIB_LANGAGE
    #REQUIERS_BIB_MATH
    #REQUIERS_BIB_MOULTFILAGE
    #REQUIERS_BIB_OBJETS
    #REQUIERS_BIB_OPENCV
    REQUIERS_BIB_OPENGL
    REQUIERS_BIB_PATRONS_CONCEPTION
    #REQUIERS_BIB_PHYS
    #REQUIERS_BIB_SYSTEME_FICHIER
    REQUIERS_BIB_TEXTURE
    REQUIERS_BIB_VISION
    #REQUIERS_BIB_XML
    #REQUIERS_KELVINLET
    #REQUIERS_PATATE
    #REQUIERS_TBB
    #REQUIERS_VORO
)

active_dependances(AVEC_DEPLOIE REQUIERS_BIB_JSON REQUIERS_BIB_TORI)

active_dependances(AVEC_HIKAGE REQUIERS_BIB_EGO)

active_dependances(AVEC_IMAGO REQUIERS_BIB_EGO REQUIERS_BIB_SYSTEME_FICHIER)

active_dependances(AVEC_KUMO REQUIERS_BIB_LANGAGE)

active_dependances(AVEC_KYANBASU REQUIERS_BIB_EGO)

active_dependances(
    AVEC_KURI

    AVEC_DANJO
    REQUIERS_BIB_CHRONO
    REQUIERS_BIB_JSON
    REQUIERS_BIB_LANGAGE
    REQUIERS_BIB_MATH
    REQUIERS_BIB_MEMOIRE
    REQUIERS_BIB_MOULTFILAGE
    REQUIERS_BIB_NOMBRE_DECIMAUX
    REQUIERS_BIB_OUTILS
    REQUIERS_BIB_PATRONS_CONCEPTION
    REQUIERS_BIB_STRUCTURES
    REQUIERS_BIB_SYSTEME_FICHIER
    REQUIERS_BIB_TESTS
    REQUIERS_BIB_TORI
)

active_dependances(
    AVEC_SILVATHEQUE

    AVEC_DANJO
    REQUIERS_BIB_IMAGE
    REQUIERS_BIB_MATH
    REQUIERS_BIB_OBJETS
    REQUIERS_BIB_OPENGL
    REQUIERS_BIB_PATRONS_CONCEPTION
    REQUIERS_BIB_TEXTURE
    REQUIERS_BIB_VISION
)

active_dependances(AVEC_MESURE REQUIERS_BIB_LANGAGE)

active_dependances(
    AVEC_TESTS

    REQUIERS_BIB_MATH
    REQUIERS_BIB_SYSTEME_FICHIER
    REQUIERS_BIB_TESTS
    REQUIERS_BIB_TYPES
)

active_dependances(
    AVEC_KANBA

    AVEC_DANJO
    REQUIERS_BIB_BRUIT
    REQUIERS_BIB_IMAGE
    REQUIERS_BIB_MATH
    REQUIERS_BIB_PHYS
    REQUIERS_BIB_OBJETS
    REQUIERS_BIB_OPENGL
    REQUIERS_BIB_PATRONS_CONCEPTION
    REQUIERS_BIB_TESTS
    REQUIERS_BIB_TEXTURE
    REQUIERS_BIB_VISION
)

active_dependances(
    AVEC_DANJO

    REQUIERS_QT
    REQUIERS_BIB_LANGAGE
    REQUIERS_BIB_PHYS
    REQUIERS_BIB_TESTS
)

# ------------------------------------------------------------------------------
# Active les dépendances des bibliothèques.

active_dependances(REQUIERS_BIB_OPENGL REQUIERS_BIB_EGO)
active_dependances(REQUIERS_BIB_EGO REQUIERS_OPENGL)

# ------------------------------------------------------------------------------

if(AVEC_KANBA)
    #find_package(PTex REQUIRED)
endif()

if(AVEC_JORJALA)
#    find_package(Alembic)
#    find_package(Bullet)
#    find_package(OpenColorIO)
#    find_package(OpenEXR)
#    add_definitions(-DAVEC_OPENEXR)
#    find_package(OpenSubDiv)
#    find_package(OpenVDB)
endif(AVEC_JORJALA)

if(AVEC_DEPLOIE)
    find_package(Curl REQUIRED)
endif(AVEC_DEPLOIE)

if(REQUIERS_QT)
    find_package(Qt6 COMPONENTS Core Gui OpenGL Widgets Sql Network)

    if(Qt6_FOUND)
        set(BIBLIOTHEQUES_QT5_CORE Qt::Core)
        set(BIBLIOTHEQUES_QT5_GUI Qt::Gui)
        set(BIBLIOTHEQUES_QT5_OPENGL Qt::OpenGL)
        set(BIBLIOTHEQUES_QT5_WIDGETS Qt::Widgets)
        set(BIBLIOTHEQUES_QT5_SQL Qt::Sql)
        set(BIBLIOTHEQUES_QT5_NETWORK Qt::Network)
    else()
        # Teste QT 5 sinon.
        find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL Widgets Sql Network)
        set(BIBLIOTHEQUES_QT_CORE Qt5::Core)
        set(BIBLIOTHEQUES_QT_GUI Qt5::Gui)
        set(BIBLIOTHEQUES_QT_OPENGL Qt5::OpenGL)
        set(BIBLIOTHEQUES_QT_WIDGETS Qt5::Widgets)
        set(BIBLIOTHEQUES_QT_SQL Qt5::Sql)
        set(BIBLIOTHEQUES_QT_NETWORK Qt5::Network)
    endif()

    set(BIBLIOTHEQUES_QT "${BIBLIOTHEQUES_QT_CORE}" "${BIBLIOTHEQUES_QT_GUI}" "${BIBLIOTHEQUES_QT_OPENGL}" "${BIBLIOTHEQUES_QT_WIDGETS}" "${BIBLIOTHEQUES_QT_SQL}" "${BIBLIOTHEQUES_QT_NETWORK}")
endif()

if(REQUIERS_OPENGL)
    set(OpenGL_GL_PREFERENCE "LEGACY")

    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)

    set(BIBLIOTHEQUES_OPENGL ${GLEW_LIBRARIES} OpenGL::GL OpenGL::GLU)
endif()

if(REQUIERS_TBB)
    find_package(TBB REQUIRED)
    set(BIBLIOTHEQUES_TBB TBB::tbb)
endif()

add_definitions(-DAVEC_JPEG)
set(BIBLIOTHEQUES_IMAGE ${BIBLIOTHEQUES_IMAGE} jpeg png)

# À FAIRE : find_package
if(REQUIERS_BOOST)
    set(INCLUSIONS_BOOST /opt/lib/boost/include)
endif()

if(AVEC_KURI)
    find_package(LibFFI REQUIRED)
endif()

# À FAIRE : find_package
if(REQUIERS_CGAL)
	set(INCLUSIONS_CGAL /opt/lib/cgal/include)
endif()

# ------------------------------------------------------------------------------

# Si nous compilons avec g++ nous devons lier la bibliothèque stdc++fs pour pouvoir
# utiliser filesystem.
set(BIBLIOTHEQUE_CXX_FILESYSTEM $<$<CXX_COMPILER_ID:GNU>:stdc++fs>)

# dl n'est disponible que sur linux
set(BIBLIOTHEQUE_DL $<$<CXX_COMPILER_ID:GNU>:dl>)

# ------------------------------------------------------------------------------

macro(installe_dans_module_kuri __nom_archive__ __nom_dossier__)
    install(TARGETS ${__nom_archive__} DESTINATION kuri/modules/${__nom_dossier__}/lib/x86_64-linux-gnu/)
endmacro(installe_dans_module_kuri)

# ------------------------------------------------------------------------------

include(platform_unix)

add_subdirectory(biblexternes)
add_subdirectory(biblinternes)
add_subdirectory(logiciels)

if(AVEC_TESTS)
    add_subdirectory(tests)
endif(AVEC_TESTS)
