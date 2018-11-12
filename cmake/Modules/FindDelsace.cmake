# Essaye de trouver les composants de la bibliothèque C++ de Delsace.
#
# Les variables suivantes seront définies pour chaque composant
#  ${COMPOSANT}_FOUND                Le système possède le composant.
#  INCLUSIONS_DELSACE_${COMPOSANT}   Les dossiers d'inclusions du composant.
#  BIBLIOTHEQUE_DELSACE_${COMPOSANT} La bibliothèque du composant.
#
# Si DOSSIER_RACINE_DELSACE se trouve dans l'environnement, on l'utilise.

if(NOT DOSSIER_RACINE_DELSACE AND NOT $ENV{DOSSIER_RACINE_DELSACE} STREQUAL "")
	set(DOSSIER_RACINE_DELSACE $ENV{DOSSIER_RACINE_DELSACE})
endif()

set(_Delsace_SEARCH_DIRS
	${DOSSIER_RACINE_DELSACE}
	/opt/lib/delsace
)

set(FICHIER_chrono chronometrage.hh)
set(FICHIER_tests test_unitaire.hh)

include(FindPackageHandleStandardArgs)

foreach(COMPOSANT ${Delsace_FIND_COMPONENTS})
	string(TOUPPER ${COMPOSANT} COMPOSANT_MAJ)

	find_path(_INCLUSIONS_DELSACE_${COMPOSANT_MAJ}
		NAMES
		    ${COMPOSANT}/${FICHIER_${COMPOSANT}}
		HINTS
		    ${_Delsace_SEARCH_DIRS}
		PATH_SUFFIXES
		    include/delsace
	)

    find_library(_BIBLIOTHEQUE_DELSACE_${COMPOSANT_MAJ}
		NAMES
		    ${COMPOSANT}
		HINTS
		    ${_Delsace_SEARCH_DIRS}
		PATH_SUFFIXES
		    lib64 lib
	)

    FIND_PACKAGE_HANDLE_STANDARD_ARGS(
		${COMPOSANT_MAJ}
		DEFAULT_MSG
		BIBLIOTHEQUE_DELSACE_${COMPOSANT_MAJ}
		INCLUSIONS_DELSACE_${COMPOSANT_MAJ})

	if(${COMPOSANT_MAJ}_FOUND)
		set(BIBLIOTHEQUE_DELSACE_${COMPOSANT_MAJ} ${_BIBLIOTHEQUE_DELSACE_${COMPOSANT_MAJ}})
		set(INCLUSIONS_DELSACE_${COMPOSANT_MAJ} ${_INCLUSIONS_DELSACE_${COMPOSANT_MAJ}} ${_INCLUSIONS_DELSACE_${COMPOSANT_MAJ}}/../)
	else()
		set(${COMPOSANT_MAJ}_FOUND FALSE)
	endif()

	mark_as_advanced(BIBLIOTHEQUE_DELSACE_${COMPOSANT_MAJ})
	mark_as_advanced(INCLUSIONS_DELSACE_${COMPOSANT_MAJ})
endforeach()

unset(_Delsace_SEARCH_DIRS)
unset(FICHIER_chrono)
unset(FICHIER_tests)
