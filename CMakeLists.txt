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

option(SEULEMENT_KURI "Ne compiler que Kuri et ses dépendances" OFF)

# ------------------------------------------------------------------------------

# Évite de polluer les options pour l'instant.
set(REQUIERS_QT OFF)
set(REQUIERS_TBB ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_OPENGL ON) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_BOOST ON)
set(REQUIERS_DOCOPT OFF) # 'ON' par défaut pour l'instant car besoin dans les biblinternes
set(REQUIERS_KELVINLET OFF)
set(REQUIERS_PATATE OFF)
set(REQUIERS_VORO OFF)
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
    set(AVEC_KANBA OFF)
    set(AVEC_KUMO OFF)
    set(AVEC_KYANBASU OFF)
    set(AVEC_MESURE OFF)
    # set(AVEC_JORJALA OFF)
    set(REQUIERS_QT OFF)
    set(AVEC_SILVATHEQUE OFF)
    set(AVEC_TESTS OFF)
    set(AVEC_VULKAN OFF)

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

# À FAIRE : permet la déclaration de plusiers dépendances
macro(active_dependances __test__ __dep__)
    if (${__test__})
        set(${__dep__} ON)
    endif()
endmacro()

active_dependances(AVEC_KANBA AVEC_DANJO)
active_dependances(AVEC_JORJALA AVEC_DANJO)
active_dependances(AVEC_SILVATHEQUE AVEC_DANJO)

#active_dependances(AVEC_JORJALA REQUIERS_TBB)

active_dependances(AVEC_KANBA REQUIERS_OPENGL)
#active_dependances(AVEC_JORJALA REQUIERS_OPENGL)
active_dependances(AVEC_SILVATHEQUE REQUIERS_OPENGL)
active_dependances(AVEC_VULKAN REQUIERS_OPENGL)

#active_dependances(AVEC_JORJALA REQUIERS_KELVINLET)
#active_dependances(AVEC_JORJALA REQUIERS_PATATE)
#active_dependances(AVEC_JORJALA REQUIERS_VORO)

active_dependances(AVEC_DANJO REQUIERS_QT)
active_dependances(AVEC_DANJO REQUIERS_BIB_LANGAGE)
active_dependances(AVEC_DANJO REQUIERS_BIB_PHYS)
active_dependances(AVEC_DANJO REQUIERS_BIB_TESTS)

active_dependances(AVEC_DEPLOIE REQUIERS_BIB_JSON)
active_dependances(AVEC_DEPLOIE REQUIERS_BIB_TORI)

active_dependances(AVEC_HIKAGE REQUIERS_BIB_EGO)

active_dependances(AVEC_IMAGO REQUIERS_BIB_EGO)
active_dependances(AVEC_IMAGO REQUIERS_BIB_SYSTEME_FICHIER)

active_dependances(AVEC_KUMO REQUIERS_BIB_LANGAGE)

active_dependances(AVEC_KYANBASU REQUIERS_BIB_EGO)

active_dependances(AVEC_KURI REQUIERS_BIB_CHRONO)
active_dependances(AVEC_KURI REQUIERS_BIB_JSON)
active_dependances(AVEC_KURI REQUIERS_BIB_LANGAGE)
active_dependances(AVEC_KURI REQUIERS_BIB_MATH)
active_dependances(AVEC_KURI REQUIERS_BIB_MEMOIRE)
active_dependances(AVEC_KURI REQUIERS_BIB_MOULTFILAGE)
active_dependances(AVEC_KURI REQUIERS_BIB_NOMBRE_DECIMAUX)
active_dependances(AVEC_KURI REQUIERS_BIB_OUTILS)
active_dependances(AVEC_KURI REQUIERS_BIB_STRUCTURES)
active_dependances(AVEC_KURI REQUIERS_BIB_SYSTEME_FICHIER)
active_dependances(AVEC_KURI REQUIERS_BIB_TESTS)
active_dependances(AVEC_KURI REQUIERS_BIB_TORI)

active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_EGO)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_IMAGE)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_MATH)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_OBJETS)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_OPENGL)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_PATRONS_CONCEPTION)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_TEXTURE)
active_dependances(AVEC_SILVATHEQUE REQUIERS_BIB_VISION)

active_dependances(AVEC_MESURE REQUIERS_BIB_LANGAGE)

active_dependances(AVEC_TESTS REQUIERS_BIB_MATH)
active_dependances(AVEC_TESTS REQUIERS_BIB_SYSTEME_FICHIER)
active_dependances(AVEC_TESTS REQUIERS_BIB_TESTS)
active_dependances(AVEC_TESTS REQUIERS_BIB_TYPES)

# ------------------------------------------------------------------------------

if(AVEC_KANBA)
    find_package(PTex REQUIRED)

    set(REQUIERS_BIB_BRUIT ON)
    set(REQUIERS_BIB_EGO ON)
    set(REQUIERS_BIB_IMAGE ON)
    set(REQUIERS_BIB_MATH ON)
    set(REQUIERS_BIB_PHYS ON)
    set(REQUIERS_BIB_OBJETS ON)
    set(REQUIERS_BIB_OPENGL ON)
    set(REQUIERS_BIB_PATRONS_CONCEPTION ON)
    set(REQUIERS_BIB_TESTS ON)
    set(REQUIERS_BIB_TEXTURE ON)
    set(REQUIERS_BIB_VISION ON)
endif()

if(AVEC_JORJALA)
#    find_package(Alembic)
#    find_package(Bullet)
#    find_package(OpenColorIO)
#    find_package(OpenEXR)
#    add_definitions(-DAVEC_OPENEXR)
#    find_package(OpenSubDiv)
#    find_package(OpenVDB)

#    set(REQUIERS_BIB_BRUIT ON)
    set(REQUIERS_BIB_EGO ON)
    set(REQUIERS_BIB_IMAGE ON)
#    set(REQUIERS_BIB_LANGAGE ON)
#    set(REQUIERS_BIB_MATH ON)
#    set(REQUIERS_BIB_MOULTFILAGE ON)
#    set(REQUIERS_BIB_OBJETS ON)
#    set(REQUIERS_BIB_OPENCV ON)
    set(REQUIERS_BIB_OPENGL ON)
    set(REQUIERS_BIB_PATRONS_CONCEPTION ON)
#    set(REQUIERS_BIB_PHYS ON)
#    set(REQUIERS_BIB_SYSTEME_FICHIER ON)
    set(REQUIERS_BIB_TEXTURE ON)
    set(REQUIERS_BIB_VISION ON)
#    set(REQUIERS_BIB_XML ON)
endif(AVEC_JORJALA)

if(AVEC_DEPLOIE)
    find_package(Curl REQUIRED)
endif(AVEC_DEPLOIE)

if(REQUIERS_QT)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL Widgets Sql)
    set(BIBLIOTHEQUES_QT_CORE Qt5::Core)
    set(BIBLIOTHEQUES_QT_GUI Qt5::Gui)
    set(BIBLIOTHEQUES_QT_OPENGL Qt5::OpenGL)
    set(BIBLIOTHEQUES_QT_WIDGETS Qt5::Widgets)
    set(BIBLIOTHEQUES_QT_SQL Qt5::Sql)

    set(BIBLIOTHEQUES_QT "${BIBLIOTHEQUES_QT_CORE}" "${BIBLIOTHEQUES_QT_GUI}" "${BIBLIOTHEQUES_QT_OPENGL}" "${BIBLIOTHEQUES_QT_WIDGETS}" "${BIBLIOTHEQUES_QT_SQL}")
endif()

# À FAIRE : find_package
if(REQUIERS_OPENGL)
    set(BIBLIOTHEQUES_OPENGL GLEW GLU GL) #glut
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

# ------------------------------------------------------------------------------

# Si nous compilons avec g++ nous devons lier la bibliothèque stdc++fs pour pouvoir
# utiliser filesystem.
set(BILIOTHEQUE_CXX_FILESYSTEM $<$<CXX_COMPILER_ID:GNU>:stdc++fs>)

# dl n'est disponible que sur linux
set(BIBLIOTHEQUE_DL $<$<CXX_COMPILER_ID:GNU>:dl>)

# ------------------------------------------------------------------------------

include(platform_unix)

add_subdirectory(biblexternes)
add_subdirectory(biblinternes)
add_subdirectory(logiciels)

if(AVEC_TESTS)
    add_subdirectory(tests)
endif(AVEC_TESTS)
