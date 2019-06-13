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

namespace arachne {

const char *chaine_identifiant(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_BASE_DE_DONNEES:
			return "IDENTIFIANT_BASE_DE_DONNÉES";
		case IDENTIFIANT_CHARGE:
			return "IDENTIFIANT_CHARGE";
		case IDENTIFIANT_CREE:
			return "IDENTIFIANT_CRÉE";
		case IDENTIFIANT_ET:
			return "IDENTIFIANT_ET";
		case IDENTIFIANT_FAUX:
			return "IDENTIFIANT_FAUX";
		case IDENTIFIANT_FICHIER:
			return "IDENTIFIANT_FICHIER";
		case IDENTIFIANT_NUL:
			return "IDENTIFIANT_NUL";
		case IDENTIFIANT_OU:
			return "IDENTIFIANT_OU";
		case IDENTIFIANT_RETOURNE:
			return "IDENTIFIANT_RETOURNE";
		case IDENTIFIANT_SI:
			return "IDENTIFIANT_SI";
		case IDENTIFIANT_SUPPRIME:
			return "IDENTIFIANT_SUPPRIME";
		case IDENTIFIANT_TROUVE:
			return "IDENTIFIANT_TROUVE";
		case IDENTIFIANT_UTILISE:
			return "IDENTIFIANT_UTILISE";
		case IDENTIFIANT_VRAI:
			return "IDENTIFIANT_VRAI";
		case IDENTIFIANT_ECRIT:
			return "IDENTIFIANT_ÉCRIT";
		case IDENTIFIANT_DIFFERENCE:
			return "IDENTIFIANT_DIFFERENCE";
		case IDENTIFIANT_INFERIEUR_EGAL:
			return "IDENTIFIANT_INFERIEUR_EGAL";
		case IDENTIFIANT_EGALITE:
			return "IDENTIFIANT_EGALITE";
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return "IDENTIFIANT_SUPERIEUR_EGAL";
		case IDENTIFIANT_GUILLEMET:
			return "IDENTIFIANT_GUILLEMET";
		case IDENTIFIANT_APOSTROPHE:
			return "IDENTIFIANT_APOSTROPHE";
		case IDENTIFIANT_PARENTHESE_OUVRANTE:
			return "IDENTIFIANT_PARENTHESE_OUVRANTE";
		case IDENTIFIANT_PARENTHESE_FERMANTE:
			return "IDENTIFIANT_PARENTHESE_FERMANTE";
		case IDENTIFIANT_VIRGULE:
			return "IDENTIFIANT_VIRGULE";
		case IDENTIFIANT_POINT:
			return "IDENTIFIANT_POINT";
		case IDENTIFIANT_DOUBLE_POINT:
			return "IDENTIFIANT_DOUBLE_POINT";
		case IDENTIFIANT_POINT_VIRGULE:
			return "IDENTIFIANT_POINT_VIRGULE";
		case IDENTIFIANT_INFERIEUR:
			return "IDENTIFIANT_INFERIEUR";
		case IDENTIFIANT_SUPERIEUR:
			return "IDENTIFIANT_SUPERIEUR";
		case IDENTIFIANT_CROCHET_OUVRANT:
			return "IDENTIFIANT_CROCHET_OUVRANT";
		case IDENTIFIANT_CROCHET_FERMANT:
			return "IDENTIFIANT_CROCHET_FERMANT";
		case IDENTIFIANT_ACCOLADE_OUVRANTE:
			return "IDENTIFIANT_ACCOLADE_OUVRANTE";
		case IDENTIFIANT_ACCOLADE_FERMANTE:
			return "IDENTIFIANT_ACCOLADE_FERMANTE";
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
	}
	return "NULL";
}

} /* namespace arachne */
