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
	${INCLUSIONS_SYSTEMES}
)

qt5_wrap_cpp(
	moc_headers

	controles/action.h
	controles/bouton.h
	controles/controle_courbe_couleur.h
	controles/controle_echelle_valeur.h
	controles/controle_nombre_decimal.h
	controles/controle_nombre_entier.h
	controles/controle_rampe_couleur.h

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
	controles_proprietes/controle_propriete_rampe_couleur.h
	controles_proprietes/controle_propriete_liste.h
	controles_proprietes/controle_propriete_vecteur.h

	conteneur_controles.h
	menu_interrogeable.h
)

set(ENTETES_TYPES
	types/courbe_bezier.h
	types/rampe_couleur.h
)

set(ENTETES
	conteneur_controles.h
	erreur.h
	danjo.h
	manipulable.h
	menu_interrogeable.h
	repondant_bouton.h
)

add_library(danjo STATIC
	controles/action.cc
	controles/bouton.cc
	controles/controle_courbe_couleur.cc
	controles/controle_echelle_valeur.cc
	controles/controle_masque.cc
	controles/controle_nombre_decimal.cc
	controles/controle_nombre_entier.cc
	controles/controle_rampe_couleur.cc
	controles/controle_roue_couleur.cc
	controles/controle_spectre_couleur.cc
	controles/controle_teinte_couleur.cc

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

	types/courbe_bezier.cc
	types/rampe_couleur.cc

	conteneur_controles.cc
	dialogue.cc
	erreur.cc
	danjo.cc
	manipulable.cc
	menu_interrogeable.cc
	repondant_bouton.cc

	controles/action.h
	controles/bouton.h
	controles/controle_courbe_couleur.h
	controles/controle_echelle_valeur.h
	controles/controle_masque.h
	controles/controle_nombre_decimal.h
	controles/controle_nombre_entier.h
	controles/controle_rampe_couleur.h
	controles/controle_roue_couleur.h
	controles/controle_spectre_couleur.h
	controles/controle_teinte_couleur.h

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

	${ENTETES_TYPES}

	dialogue.h
	${ENTETES}

	${moc_headers}
)

target_include_directories(danjo PUBLIC "${INCLUSIONS}")

target_link_libraries(danjo "${BIBLIOTHEQUES}")

install(TARGETS danjo ARCHIVE DESTINATION lib)

install(
	FILES ${ENTETES}
	DESTINATION include/danjo
)

install(
	FILES ${ENTETES_TYPES}
	DESTINATION include/danjo/types
)
