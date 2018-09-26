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

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#include "morceaux.h"

#include <algorithm>

struct paire_identifiant_chaine {
	int identifiant;
	std::string chaine;
};

struct paire_identifiant_caractere {
	int identifiant;
	char caractere;
};

static paire_identifiant_chaine paires_mots_cles[] = {
	{ID_ARRETE, "arrête"},
	{ID_ASSOCIE, "associe"},
	{ID_BOOL, "bool"},
	{ID_BOUCLE, "boucle"},
	{ID_CONSTANTE, "constante"},
	{ID_CONTINUE, "continue"},
	{ID_DANS, "dans"},
	{ID_DE, "de"},
	{ID_DEFERE, "défère"},
	{ID_E16, "e16"},
	{ID_E16NS, "e16ns"},
	{ID_E32, "e32"},
	{ID_E32NS, "e32ns"},
	{ID_E64, "e64"},
	{ID_E64NS, "e64ns"},
	{ID_E8, "e8"},
	{ID_E8NS, "e8ns"},
	{ID_FAUX, "faux"},
	{ID_FONCTION, "fonction"},
	{ID_GABARIT, "gabarit"},
	{ID_POUR, "pour"},
	{ID_R16, "r16"},
	{ID_R32, "r32"},
	{ID_R64, "r64"},
	{ID_RETOURNE, "retourne"},
	{ID_RIEN, "rien"},
	{ID_SI, "si"},
	{ID_SINON, "sinon"},
	{ID_SOIT, "soit"},
	{ID_STRUCTURE, "structure"},
	{ID_TRANSTYPE, "transtype"},
	{ID_VARIABLE, "variable"},
	{ID_VRAI, "vrai"},
	{ID_ENUM, "énum"},
};

static paire_identifiant_chaine paires_caracteres_double[] = {
	{ID_DIFFERENCE, "!="},
	{ID_ESP_ESP, "&&"},
	{ID_DECALAGE_GAUCHE, "<<"},
	{ID_INFERIEUR_EGAL, "<="},
	{ID_EGALITE, "=="},
	{ID_SUPERIEUR_EGAL, ">="},
	{ID_DECALAGE_DROITE, ">>"},
	{ID_BARRE_BARRE, "||"},
};

static paire_identifiant_caractere paires_caracteres_speciaux[] = {
	{ID_EXCLAMATION, '!'},
	{ID_GUILLEMET, '"'},
	{ID_DIESE, '#'},
	{ID_POURCENT, '%'},
	{ID_ESPERLUETTE, '&'},
	{ID_APOSTROPHE, '\''},
	{ID_PARENTHESE_OUVRANTE, '('},
	{ID_PARENTHESE_FERMANTE, ')'},
	{ID_FOIS, '*'},
	{ID_PLUS, '+'},
	{ID_VIRGULE, ','},
	{ID_MOINS, '-'},
	{ID_POINT, '.'},
	{ID_DIVISE, '/'},
	{ID_DOUBLE_POINTS, ':'},
	{ID_POINT_VIRGULE, ';'},
	{ID_INFERIEUR, '<'},
	{ID_EGAL, '='},
	{ID_SUPERIEUR, '>'},
	{ID_AROBASE, '@'},
	{ID_CROCHET_OUVRANT, '['},
	{ID_CROCHET_FERMANT, ']'},
	{ID_CHAPEAU, '^'},
	{ID_ACCOLADE_OUVRANTE, '{'},
	{ID_BARRE, '|'},
	{ID_ACCOLADE_FERMANTE, '}'},
	{ID_TILDE, '~'},
};
const char *chaine_identifiant(int id)
{
	switch (id) {
		case ID_EXCLAMATION:
			return "ID_EXCLAMATION";
		case ID_GUILLEMET:
			return "ID_GUILLEMET";
		case ID_DIESE:
			return "ID_DIESE";
		case ID_POURCENT:
			return "ID_POURCENT";
		case ID_ESPERLUETTE:
			return "ID_ESPERLUETTE";
		case ID_APOSTROPHE:
			return "ID_APOSTROPHE";
		case ID_PARENTHESE_OUVRANTE:
			return "ID_PARENTHESE_OUVRANTE";
		case ID_PARENTHESE_FERMANTE:
			return "ID_PARENTHESE_FERMANTE";
		case ID_FOIS:
			return "ID_FOIS";
		case ID_PLUS:
			return "ID_PLUS";
		case ID_VIRGULE:
			return "ID_VIRGULE";
		case ID_MOINS:
			return "ID_MOINS";
		case ID_POINT:
			return "ID_POINT";
		case ID_DIVISE:
			return "ID_DIVISE";
		case ID_DOUBLE_POINTS:
			return "ID_DOUBLE_POINTS";
		case ID_POINT_VIRGULE:
			return "ID_POINT_VIRGULE";
		case ID_INFERIEUR:
			return "ID_INFERIEUR";
		case ID_EGAL:
			return "ID_EGAL";
		case ID_SUPERIEUR:
			return "ID_SUPERIEUR";
		case ID_AROBASE:
			return "ID_AROBASE";
		case ID_CROCHET_OUVRANT:
			return "ID_CROCHET_OUVRANT";
		case ID_CROCHET_FERMANT:
			return "ID_CROCHET_FERMANT";
		case ID_CHAPEAU:
			return "ID_CHAPEAU";
		case ID_ACCOLADE_OUVRANTE:
			return "ID_ACCOLADE_OUVRANTE";
		case ID_BARRE:
			return "ID_BARRE";
		case ID_ACCOLADE_FERMANTE:
			return "ID_ACCOLADE_FERMANTE";
		case ID_TILDE:
			return "ID_TILDE";
		case ID_DIFFERENCE:
			return "ID_DIFFERENCE";
		case ID_ESP_ESP:
			return "ID_ESP_ESP";
		case ID_DECALAGE_GAUCHE:
			return "ID_DECALAGE_GAUCHE";
		case ID_INFERIEUR_EGAL:
			return "ID_INFERIEUR_EGAL";
		case ID_EGALITE:
			return "ID_EGALITE";
		case ID_SUPERIEUR_EGAL:
			return "ID_SUPERIEUR_EGAL";
		case ID_DECALAGE_DROITE:
			return "ID_DECALAGE_DROITE";
		case ID_BARRE_BARRE:
			return "ID_BARRE_BARRE";
		case ID_ARRETE:
			return "ID_ARRETE";
		case ID_ASSOCIE:
			return "ID_ASSOCIE";
		case ID_BOOL:
			return "ID_BOOL";
		case ID_BOUCLE:
			return "ID_BOUCLE";
		case ID_CONSTANTE:
			return "ID_CONSTANTE";
		case ID_CONTINUE:
			return "ID_CONTINUE";
		case ID_DANS:
			return "ID_DANS";
		case ID_DE:
			return "ID_DE";
		case ID_DEFERE:
			return "ID_DEFERE";
		case ID_E16:
			return "ID_E16";
		case ID_E16NS:
			return "ID_E16NS";
		case ID_E32:
			return "ID_E32";
		case ID_E32NS:
			return "ID_E32NS";
		case ID_E64:
			return "ID_E64";
		case ID_E64NS:
			return "ID_E64NS";
		case ID_E8:
			return "ID_E8";
		case ID_E8NS:
			return "ID_E8NS";
		case ID_FAUX:
			return "ID_FAUX";
		case ID_FONCTION:
			return "ID_FONCTION";
		case ID_GABARIT:
			return "ID_GABARIT";
		case ID_POUR:
			return "ID_POUR";
		case ID_R16:
			return "ID_R16";
		case ID_R32:
			return "ID_R32";
		case ID_R64:
			return "ID_R64";
		case ID_RETOURNE:
			return "ID_RETOURNE";
		case ID_RIEN:
			return "ID_RIEN";
		case ID_SI:
			return "ID_SI";
		case ID_SINON:
			return "ID_SINON";
		case ID_SOIT:
			return "ID_SOIT";
		case ID_STRUCTURE:
			return "ID_STRUCTURE";
		case ID_TRANSTYPE:
			return "ID_TRANSTYPE";
		case ID_VARIABLE:
			return "ID_VARIABLE";
		case ID_VRAI:
			return "ID_VRAI";
		case ID_ENUM:
			return "ID_ENUM";
		case ID_NOMBRE_REEL:
			return "ID_NOMBRE_REEL";
		case ID_NOMBRE_ENTIER:
			return "ID_NOMBRE_ENTIER";
		case ID_NOMBRE_HEXADECIMAL:
			return "ID_NOMBRE_HEXADECIMAL";
		case ID_NOMBRE_OCTAL:
			return "ID_NOMBRE_OCTAL";
		case ID_NOMBRE_BINAIRE:
			return "ID_NOMBRE_BINAIRE";
		case ID_TROIS_POINTS:
			return "ID_TROIS_POINTS";
		case ID_CHAINE_CARACTERE:
			return "ID_CHAINE_CARACTERE";
		case ID_CHAINE_LITTERALE:
			return "ID_CHAINE_LITTERALE";
		case ID_CARACTERE:
			return "ID_CARACTERE";
		case ID_INCONNU:
			return "ID_INCONNU";
	};

	return "ERREUR";
}


static bool comparaison_paires_identifiant(
		const paire_identifiant_chaine &a,
		const paire_identifiant_chaine &b)
{
	return a.chaine < b.chaine;
}

static bool comparaison_paires_caractere(
		const paire_identifiant_caractere &a,
		const paire_identifiant_caractere &b)
{
	return a.caractere < b.caractere;
}

bool est_caractere_special(char c, int &i)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_caracteres_speciaux),
						 std::end(paires_caracteres_speciaux),
						 paire_identifiant_caractere{ID_INCONNU, c},
						 comparaison_paires_caractere);

	if (iterateur != std::end(paires_caracteres_speciaux)) {
		if ((*iterateur).caractere == c) {
			i = (*iterateur).identifiant;
			return true;
		}
	}

	return false;
}

int id_caractere_double(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_caracteres_double),
						 std::end(paires_caracteres_double),
						 paire_identifiant_chaine{ID_INCONNU, chaine},
						 comparaison_paires_identifiant);

	if (iterateur != std::end(paires_caracteres_double)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return ID_INCONNU;
}

int id_chaine(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_mots_cles),
						 std::end(paires_mots_cles),
						 paire_identifiant_chaine{ID_INCONNU, chaine},
						 comparaison_paires_identifiant);

	if (iterateur != std::end(paires_mots_cles)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return ID_CHAINE_CARACTERE;
}
