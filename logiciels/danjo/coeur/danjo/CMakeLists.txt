# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2018 Kévin Dietrich.
# All rights reserved.
#
# ***** END GPL LICENSE BLOCK *****

set(INCLUSIONS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../
	${INCLUSIONS_SYSTEMES}
)

set(ENTETES_POUR_QT
    controles/action.h
    controles/base_controle.hh
    controles/bouton.h
    controles/controle_couleur.h
    controles/controle_couleur_tsv.h
    controles/controle_courbe_couleur.h
    controles/controle_echelle_valeur.h
    controles/controle_nombre_decimal.h
    controles/controle_nombre_entier.h
    controles/controle_rampe_couleur.h
    controles/menu_filtrable.hh

    controles_proprietes/controle_propriete.h
    controles_proprietes/controle_propriete_bool.h
    controles_proprietes/controle_propriete_chaine.h
    controles_proprietes/controle_propriete_couleur.h
    controles_proprietes/controle_propriete_courbe_couleur.h
    controles_proprietes/controle_propriete_courbe_valeur.h
    controles_proprietes/controle_propriete_decimal.h
    controles_proprietes/controle_propriete_entier.h
    controles_proprietes/controle_propriete_enum.h
    controles_proprietes/controle_propriete_fichier.h
    controles_proprietes/controle_propriete_rampe_couleur.h
    controles_proprietes/controle_propriete_liste.h
    controles_proprietes/controle_propriete_liste_manip.hh
    controles_proprietes/controle_propriete_vecteur.h

    dialogues/dialogue_couleur.h

    conteneur_controles.h
)

if(Qt6_FOUND)
    qt_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
else()
    qt5_wrap_cpp(moc_headers ${ENTETES_POUR_QT})
endif()

#NOTE: pour ici et plus bas (CMAKE_BUILD_TYPE MATCHES "Release"):
# lors que nous compilons en mode Debug nous devons désactiver certains drapeaux
# de compilation trop stricts pour Qt, donc nous utilisons une bibliothèque
# extra afin de pouvoir isoler les drapeaux de compilation. Mais, lorsque nous
# lions l'archive avec un autre logiciel, ld.gold se plaint que les tables
# virtuelles sont absentes. Donc nous avons une configuration Debug pour faire
# taire les erreurs de compilations, et une configuration Release où les
# Metaobjets de Qt sont générés avec le code de la bibliothèque danjo pour que
# les tables virtuelles soient correctement créées.
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Profile")
	add_library(qt_danjo_entreface ${moc_headers})
	target_include_directories(qt_danjo_entreface PUBLIC "${INCLUSIONS}")
        target_link_libraries(qt_danjo_entreface ${BIBLIOTHEQUES})

	desactive_drapeaux_compilation(qt_danjo_entreface)

	set(BIBLIOTHEQUES
		${BIBLIOTHEQUES}
		qt_danjo_entreface
	)
endif()

set(ENTETES_TYPES
	types/courbe_bezier.h
	types/liste_manip.hh
	types/outils.h
	types/rampe_couleur.h
)

set(ENTETES
	conteneur_controles.h
	erreur.h
	danjo.h
        fournisseuse_icones.hh
	manipulable.h
        proprietes.hh
        repondant_bouton.h
)

set(SOURCES
	controles/action.cc
        controles/base_controle.cc
	controles/bouton.cc
	controles/controle_couleur.cc
	controles/controle_couleur_tsv.cc
	controles/controle_courbe_couleur.cc
	controles/controle_echelle_valeur.cc
	controles/controle_masque.cc
	controles/controle_nombre_decimal.cc
	controles/controle_nombre_entier.cc
	controles/controle_rampe_couleur.cc
	controles/controle_roue_couleur.cc
	controles/controle_spectre_couleur.cc
	controles/controle_teinte_couleur.cc
	controles/menu_filtrable.cc

        controles_proprietes/commun.cc
	controles_proprietes/controle_propriete.cc
	controles_proprietes/controle_propriete_bool.cc
	controles_proprietes/controle_propriete_chaine.cc
	controles_proprietes/controle_propriete_couleur.cc
	controles_proprietes/controle_propriete_courbe_couleur.cc
	controles_proprietes/controle_propriete_courbe_valeur.cc
	controles_proprietes/controle_propriete_decimal.cc
	controles_proprietes/controle_propriete_entier.cc
	controles_proprietes/controle_propriete_enum.cc
	controles_proprietes/controle_propriete_etiquette.cc
	controles_proprietes/controle_propriete_fichier.cc
	controles_proprietes/controle_propriete_rampe_couleur.cc
	controles_proprietes/controle_propriete_liste.cc
	controles_proprietes/controle_propriete_liste_manip.cc
	controles_proprietes/controle_propriete_vecteur.cc
	controles_proprietes/donnees_controle.cc

	compilation/analyseuse_disposition.cc
	compilation/analyseuse_logique.cc
	compilation/assembleuse_disposition.cc
	compilation/assembleuse_logique.cc
	compilation/base_analyseuse.cc
	compilation/decoupeuse.cc
	compilation/expression.cc
	compilation/graphe_contrainte.cc
	compilation/morceaux.cc

	dialogues/dialogue_couleur.cc

	types/courbe_bezier.cc
	types/liste_manip.cc
	types/outils.cc
	types/rampe_couleur.cc

	conteneur_controles.cc
	dialogue.cc
	erreur.cc
	danjo.cc
        fournisseuse_icones.cc
	manipulable.cc
        proprietes.cc
	repondant_bouton.cc

	controles/action.h
        controles/base_controle.hh
	controles/bouton.h
	controles/controle_couleur.h
	controles/controle_couleur_tsv.h
	controles/controle_courbe_couleur.h
	controles/controle_echelle_valeur.h
	controles/controle_masque.h
	controles/controle_nombre_decimal.h
	controles/controle_nombre_entier.h
	controles/controle_rampe_couleur.h
	controles/controle_roue_couleur.h
	controles/controle_spectre_couleur.h
	controles/controle_teinte_couleur.h
	controles/menu_filtrable.hh

        controles_proprietes/commun.hh
	controles_proprietes/controle_propriete.h
	controles_proprietes/controle_propriete_bool.h
	controles_proprietes/controle_propriete_chaine.h
	controles_proprietes/controle_propriete_couleur.h
	controles_proprietes/controle_propriete_courbe_couleur.h
	controles_proprietes/controle_propriete_courbe_valeur.h
	controles_proprietes/controle_propriete_decimal.h
	controles_proprietes/controle_propriete_entier.h
	controles_proprietes/controle_propriete_enum.h
	controles_proprietes/controle_propriete_etiquette.h
	controles_proprietes/controle_propriete_fichier.h
	controles_proprietes/controle_propriete_liste.h
	controles_proprietes/controle_propriete_liste_manip.hh
	controles_proprietes/controle_propriete_rampe_couleur.h
	controles_proprietes/controle_propriete_vecteur.h
	controles_proprietes/donnees_controle.h

	compilation/analyseuse_disposition.h
	compilation/analyseuse_logique.h
	compilation/assembleuse_disposition.h
	compilation/assembleuse_logique.h
	compilation/base_analyseuse.h
	compilation/decoupeuse.h
	compilation/expression.h
	compilation/graphe_contrainte.h
	compilation/morceaux.h

	dialogues/dialogue_couleur.h

	${ENTETES_TYPES}

	dialogue.h
	${ENTETES}
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
	set(SOURCES
		${moc_headers}
		${SOURCES}
	)
endif()

add_library(danjo STATIC ${SOURCES})

target_include_directories(danjo PUBLIC "${INCLUSIONS}")

target_link_libraries(danjo ${BIBLIOTHEQUES})
installe_dans_module_kuri(danjo Qt)
