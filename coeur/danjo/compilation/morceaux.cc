/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "morceaux.h"

#include <sstream>

namespace danjo {

std::vector<std::string> decoupe(const std::string &chaine, const char delimiteur)
{
	std::vector<std::string> resultat;
	std::stringstream ss(chaine);
	std::string temp;

	while (std::getline(ss, temp, delimiteur)) {
		resultat.push_back(temp);
	}

	return resultat;
}

const char *chaine_identifiant(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ACTION:
			return "IDENTIFIANT_ACTION";
		case IDENTIFIANT_ATTACHE:
			return "IDENTIFIANT_ATTACHE";
		case IDENTIFIANT_BARRE_OUTILS:
			return "IDENTIFIANT_BARRE_OUTILS";
		case IDENTIFIANT_BOUTON:
			return "IDENTIFIANT_BOUTON";
		case IDENTIFIANT_CASE:
			return "IDENTIFIANT_CASE";
		case IDENTIFIANT_CHAINE:
			return "IDENTIFIANT_CHAINE";
		case IDENTIFIANT_COLONNE:
			return "IDENTIFIANT_COLONNE";
		case IDENTIFIANT_COULEUR:
			return "IDENTIFIANT_COULEUR";
		case IDENTIFIANT_DISPOSITION:
			return "IDENTIFIANT_DISPOSITION";
		case IDENTIFIANT_DOSSIER:
			return "IDENTIFIANT_DOSSIER";
		case IDENTIFIANT_DECIMAL:
			return "IDENTIFIANT_DECIMAL";
		case IDENTIFIANT_ENTIER:
			return "IDENTIFIANT_ENTIER";
		case IDENTIFIANT_ENTREE:
			return "IDENTIFIANT_ENTREE";
		case IDENTIFIANT_FAUX:
			return "IDENTIFIANT_FAUX";
		case IDENTIFIANT_FEUILLE:
			return "IDENTIFIANT_FEUILLE";
		case IDENTIFIANT_FICHIER_ENTREE:
			return "IDENTIFIANT_FICHIER_ENTREE";
		case IDENTIFIANT_FICHIER_SORTIE:
			return "IDENTIFIANT_FICHIER_SORTIE";
		case IDENTIFIANT_FILTRES:
			return "IDENTIFIANT_FILTRES";
		case IDENTIFIANT_ICONE:
			return "IDENTIFIANT_ICONE";
		case IDENTIFIANT_INFOBULLE:
			return "IDENTIFIANT_INFOBULLE";
		case IDENTIFIANT_INTERFACE:
			return "IDENTIFIANT_INTERFACE";
		case IDENTIFIANT_ITEMS:
			return "IDENTIFIANT_ITEMS";
		case IDENTIFIANT_LIGNE:
			return "IDENTIFIANT_LIGNE";
		case IDENTIFIANT_LISTE:
			return "IDENTIFIANT_LISTE";
		case IDENTIFIANT_LOGIQUE:
			return "IDENTIFIANT_LOGIQUE";
		case IDENTIFIANT_MAX:
			return "IDENTIFIANT_MAX";
		case IDENTIFIANT_MENU:
			return "IDENTIFIANT_MENU";
		case IDENTIFIANT_MIN:
			return "IDENTIFIANT_MIN";
		case IDENTIFIANT_METADONNEE:
			return "IDENTIFIANT_METADONNEE";
		case IDENTIFIANT_NOM:
			return "IDENTIFIANT_NOM";
		case IDENTIFIANT_ONGLET:
			return "IDENTIFIANT_ONGLET";
		case IDENTIFIANT_PAS:
			return "IDENTIFIANT_PAS";
		case IDENTIFIANT_PRECISION:
			return "IDENTIFIANT_PRECISION";
		case IDENTIFIANT_QUAND:
			return "IDENTIFIANT_QUAND";
		case IDENTIFIANT_RELATION:
			return "IDENTIFIANT_RELATION";
		case IDENTIFIANT_RESULTAT:
			return "IDENTIFIANT_RESULTAT";
		case IDENTIFIANT_SORTIE:
			return "IDENTIFIANT_SORTIE";
		case IDENTIFIANT_SUFFIXE:
			return "IDENTIFIANT_SUFFIXE";
		case IDENTIFIANT_SEPARATEUR:
			return "IDENTIFIANT_SEPARATEUR";
		case IDENTIFIANT_VALEUR:
			return "IDENTIFIANT_VALEUR";
		case IDENTIFIANT_VECTEUR:
			return "IDENTIFIANT_VECTEUR";
		case IDENTIFIANT_VRAI:
			return "IDENTIFIANT_VRAI";
		case IDENTIFIANT_ENUM:
			return "IDENTIFIANT_ENUM";
		case IDENTIFIANT_ETIQUETTE:
			return "IDENTIFIANT_ETIQUETTE";
		case IDENTIFIANT_DIFFERENCE:
			return "IDENTIFIANT_DIFFERENCE";
		case IDENTIFIANT_ESP_ESP:
			return "IDENTIFIANT_ESP_ESP";
		case IDENTIFIANT_ET_EGAL:
			return "IDENTIFIANT_ET_EGAL";
		case IDENTIFIANT_FOIS_EGAL:
			return "IDENTIFIANT_FOIS_EGAL";
		case IDENTIFIANT_PLUS_PLUS:
			return "IDENTIFIANT_PLUS_PLUS";
		case IDENTIFIANT_PLUS_EGAL:
			return "IDENTIFIANT_PLUS_EGAL";
		case IDENTIFIANT_MOINS_MOINS:
			return "IDENTIFIANT_MOINS_MOINS";
		case IDENTIFIANT_MOINS_EGAL:
			return "IDENTIFIANT_MOINS_EGAL";
		case IDENTIFIANT_FLECHE:
			return "IDENTIFIANT_FLECHE";
		case IDENTIFIANT_TROIS_POINT:
			return "IDENTIFIANT_TROIS_POINT";
		case IDENTIFIANT_DIVISE_EGAL:
			return "IDENTIFIANT_DIVISE_EGAL";
		case IDENTIFIANT_DECALAGE_GAUCHE:
			return "IDENTIFIANT_DECALAGE_GAUCHE";
		case IDENTIFIANT_INFERIEUR_EGAL:
			return "IDENTIFIANT_INFERIEUR_EGAL";
		case IDENTIFIANT_EGALITE:
			return "IDENTIFIANT_EGALITE";
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return "IDENTIFIANT_SUPERIEUR_EGAL";
		case IDENTIFIANT_DECALAGE_DROITE:
			return "IDENTIFIANT_DECALAGE_DROITE";
		case IDENTIFIANT_OUX_EGAL:
			return "IDENTIFIANT_OUX_EGAL";
		case IDENTIFIANT_OU_EGAL:
			return "IDENTIFIANT_OU_EGAL";
		case IDENTIFIANT_BARE_BARRE:
			return "IDENTIFIANT_BARE_BARRE";
		case IDENTIFIANT_EXCLAMATION:
			return "IDENTIFIANT_EXCLAMATION";
		case IDENTIFIANT_GUILLEMET:
			return "IDENTIFIANT_GUILLEMET";
		case IDENTIFIANT_DIESE:
			return "IDENTIFIANT_DIESE";
		case IDENTIFIANT_POURCENT:
			return "IDENTIFIANT_POURCENT";
		case IDENTIFIANT_ESPERLUETTE:
			return "IDENTIFIANT_ESPERLUETTE";
		case IDENTIFIANT_APOSTROPHE:
			return "IDENTIFIANT_APOSTROPHE";
		case IDENTIFIANT_PARENTHESE_OUVRANTE:
			return "IDENTIFIANT_PARENTHESE_OUVRANTE";
		case IDENTIFIANT_PARENTHESE_FERMANTE:
			return "IDENTIFIANT_PARENTHESE_FERMANTE";
		case IDENTIFIANT_FOIS:
			return "IDENTIFIANT_FOIS";
		case IDENTIFIANT_PLUS:
			return "IDENTIFIANT_PLUS";
		case IDENTIFIANT_VIRGULE:
			return "IDENTIFIANT_VIRGULE";
		case IDENTIFIANT_MOINS:
			return "IDENTIFIANT_MOINS";
		case IDENTIFIANT_POINT:
			return "IDENTIFIANT_POINT";
		case IDENTIFIANT_DIVISE:
			return "IDENTIFIANT_DIVISE";
		case IDENTIFIANT_DOUBLE_POINT:
			return "IDENTIFIANT_DOUBLE_POINT";
		case IDENTIFIANT_POINT_VIRGULE:
			return "IDENTIFIANT_POINT_VIRGULE";
		case IDENTIFIANT_INFERIEUR:
			return "IDENTIFIANT_INFERIEUR";
		case IDENTIFIANT_EGAL:
			return "IDENTIFIANT_EGAL";
		case IDENTIFIANT_SUPERIEUR:
			return "IDENTIFIANT_SUPERIEUR";
		case IDENTIFIANT_CROCHET_OUVRANT:
			return "IDENTIFIANT_CROCHET_OUVRANT";
		case IDENTIFIANT_CROCHET_FERMANT:
			return "IDENTIFIANT_CROCHET_FERMANT";
		case IDENTIFIANT_CHAPEAU:
			return "IDENTIFIANT_CHAPEAU";
		case IDENTIFIANT_ACCOLADE_OUVRANTE:
			return "IDENTIFIANT_ACCOLADE_OUVRANTE";
		case IDENTIFIANT_BARRE:
			return "IDENTIFIANT_BARRE";
		case IDENTIFIANT_ACCOLADE_FERMANTE:
			return "IDENTIFIANT_ACCOLADE_FERMANTE";
		case IDENTIFIANT_TILDE:
			return "IDENTIFIANT_TILDE";
		case IDENTIFIANT_CHAINE_CARACTERE:
			return "IDENTIFIANT_CHAINE_CARACTERE";
		case IDENTIFIANT_CHAINE_LITTERALE:
			return "IDENTIFIANT_CHAINE_LITTERALE";
		case IDENTIFIANT_CARACTERE:
			return "IDENTIFIANT_CARACTERE";
		case IDENTIFIANT_NOMBRE:
			return "IDENTIFIANT_NOMBRE";
		case IDENTIFIANT_NOMBRE_DECIMAL:
			return "IDENTIFIANT_NOMBRE_DECIMAL";
		case IDENTIFIANT_BOOL:
			return "IDENTIFIANT_BOOL";
		case IDENTIFIANT_NUL:
			return "IDENTIFIANT_NUL";
	}
	return "NULL";
}

}  /* namespace danjo */
