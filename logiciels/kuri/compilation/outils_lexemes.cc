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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "outils_lexemes.hh"

#include "lexemes.hh"

bool est_type_entier(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::BOOL:
		case TypeLexeme::N8:
		case TypeLexeme::N16:
		case TypeLexeme::N32:
		case TypeLexeme::N64:
		case TypeLexeme::N128:
		case TypeLexeme::Z8:
		case TypeLexeme::Z16:
		case TypeLexeme::Z32:
		case TypeLexeme::Z64:
		case TypeLexeme::Z128:
		case TypeLexeme::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_naturel(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::N8:
		case TypeLexeme::N16:
		case TypeLexeme::N32:
		case TypeLexeme::N64:
		case TypeLexeme::N128:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_relatif(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::Z8:
		case TypeLexeme::Z16:
		case TypeLexeme::Z32:
		case TypeLexeme::Z64:
		case TypeLexeme::Z128:
			return true;
		default:
			return false;
	}
}

bool est_type_reel(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::R16:
		case TypeLexeme::R32:
		case TypeLexeme::R64:
		case TypeLexeme::R128:
			return true;
		default:
			return false;
	}
}

bool est_operateur_bool(TypeLexeme type)
{
	switch (type) {
		case TypeLexeme::EXCLAMATION:
		case TypeLexeme::INFERIEUR:
		case TypeLexeme::INFERIEUR_EGAL:
		case TypeLexeme::SUPERIEUR:
		case TypeLexeme::SUPERIEUR_EGAL:
		case TypeLexeme::DIFFERENCE:
		case TypeLexeme::ESP_ESP:
		case TypeLexeme::EGALITE:
		case TypeLexeme::BARRE_BARRE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool est_assignation_operee(TypeLexeme type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case TypeLexeme::MOINS_EGAL:
		case TypeLexeme::PLUS_EGAL:
		case TypeLexeme::MULTIPLIE_EGAL:
		case TypeLexeme::DIVISE_EGAL:
		case TypeLexeme::MODULO_EGAL:
		case TypeLexeme::ET_EGAL:
		case TypeLexeme::OU_EGAL:
		case TypeLexeme::OUX_EGAL:
		case TypeLexeme::DEC_DROITE_EGAL:
		case TypeLexeme::DEC_GAUCHE_EGAL:
		{
			return true;
		}
	}
}

TypeLexeme operateur_pour_assignation_operee(TypeLexeme type)
{
	switch (type) {
		default:
		{
			return type;
		}
		case TypeLexeme::MOINS_EGAL:
		{
			return TypeLexeme::MOINS;
		}
		case TypeLexeme::PLUS_EGAL:
		{
			return TypeLexeme::PLUS;
		}
		case TypeLexeme::MULTIPLIE_EGAL:
		{
			return TypeLexeme::FOIS;
		}
		case TypeLexeme::DIVISE_EGAL:
		{
			return TypeLexeme::DIVISE;
		}
		case TypeLexeme::MODULO_EGAL:
		{
			return TypeLexeme::POURCENT;
		}
		case TypeLexeme::ET_EGAL:
		{
			return TypeLexeme::ESPERLUETTE;
		}
		case TypeLexeme::OU_EGAL:
		{
			return TypeLexeme::BARRE;
		}
		case TypeLexeme::OUX_EGAL:
		{
			return TypeLexeme::CHAPEAU;
		}
		case TypeLexeme::DEC_DROITE_EGAL:
		{
			return TypeLexeme::DECALAGE_DROITE;
		}
		case TypeLexeme::DEC_GAUCHE_EGAL:
		{
			return TypeLexeme::DECALAGE_GAUCHE;
		}
	}
}

bool est_operateur_comp(TypeLexeme type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case TypeLexeme::INFERIEUR:
		case TypeLexeme::INFERIEUR_EGAL:
		case TypeLexeme::SUPERIEUR:
		case TypeLexeme::SUPERIEUR_EGAL:
		case TypeLexeme::EGALITE:
		case TypeLexeme::DIFFERENCE:
		{
			return true;
		}
	}
}

bool peut_etre_dereference(TypeLexeme id)
{
	switch (id & 0xff) {
		default:
			return false;
		case TypeLexeme::POINTEUR:
		case TypeLexeme::REFERENCE:
		case TypeLexeme::TABLEAU:
		case TypeLexeme::TROIS_POINTS:
			return true;
	}
}

bool est_mot_cle(TypeLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case TypeLexeme::FONC:
		case TypeLexeme::STRUCT:
		case TypeLexeme::DYN:
		case TypeLexeme::SOIT:
		case TypeLexeme::RETOURNE:
		case TypeLexeme::ENUM:
		case TypeLexeme::ENUM_DRAPEAU:
		case TypeLexeme::RETIENS:
		case TypeLexeme::EXTERNE:
		case TypeLexeme::IMPORTE:
		case TypeLexeme::POUR:
		case TypeLexeme::DANS:
		case TypeLexeme::BOUCLE:
		case TypeLexeme::TANTQUE:
		case TypeLexeme::REPETE:
		case TypeLexeme::SINON:
		case TypeLexeme::SI:
		case TypeLexeme::SAUFSI:
		case TypeLexeme::LOGE:
		case TypeLexeme::DELOGE:
		case TypeLexeme::RELOGE:
		case TypeLexeme::DISCR:
		case TypeLexeme::Z8:
		case TypeLexeme::Z16:
		case TypeLexeme::Z32:
		case TypeLexeme::Z64:
		case TypeLexeme::Z128:
		case TypeLexeme::N8:
		case TypeLexeme::N16:
		case TypeLexeme::N32:
		case TypeLexeme::N64:
		case TypeLexeme::N128:
		case TypeLexeme::R16:
		case TypeLexeme::R32:
		case TypeLexeme::R64:
		case TypeLexeme::R128:
		case TypeLexeme::EINI:
		case TypeLexeme::BOOL:
		case TypeLexeme::RIEN:
		case TypeLexeme::CHAINE:
		case TypeLexeme::OCTET:
		case TypeLexeme::UNION:
		case TypeLexeme::COROUT:
		case TypeLexeme::CHARGE:
		{
			return true;
		}
	}
}

bool est_chaine_litterale(TypeLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case TypeLexeme::CHAINE_LITTERALE:
		case TypeLexeme::CARACTERE:
		{
			return true;
		}
	}
}

bool est_specifiant_type(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::FOIS:
		case TypeLexeme::ESPERLUETTE:
		case TypeLexeme::CROCHET_OUVRANT:
		case TypeLexeme::TROIS_POINTS:
		case TypeLexeme::TYPE_DE:
			return true;
		default:
			return false;
	}
}

bool est_identifiant_type(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::N8:
		case TypeLexeme::N16:
		case TypeLexeme::N32:
		case TypeLexeme::N64:
		case TypeLexeme::N128:
		case TypeLexeme::R16:
		case TypeLexeme::R32:
		case TypeLexeme::R64:
		case TypeLexeme::R128:
		case TypeLexeme::Z8:
		case TypeLexeme::Z16:
		case TypeLexeme::Z32:
		case TypeLexeme::Z64:
		case TypeLexeme::Z128:
		case TypeLexeme::BOOL:
		case TypeLexeme::RIEN:
		case TypeLexeme::EINI:
		case TypeLexeme::CHAINE:
		case TypeLexeme::CHAINE_CARACTERE:
		case TypeLexeme::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_nombre_entier(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::NOMBRE_BINAIRE:
		case TypeLexeme::NOMBRE_ENTIER:
		case TypeLexeme::NOMBRE_HEXADECIMAL:
		case TypeLexeme::NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

bool est_nombre(TypeLexeme identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == TypeLexeme::NOMBRE_REEL);
}

bool est_operateur_unaire(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::AROBASE:
		case TypeLexeme::EXCLAMATION:
		case TypeLexeme::TILDE:
		case TypeLexeme::CROCHET_OUVRANT:
		case TypeLexeme::PLUS_UNAIRE:
		case TypeLexeme::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

bool est_operateur_binaire(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::PLUS:
		case TypeLexeme::MOINS:
		case TypeLexeme::FOIS:
		case TypeLexeme::DIVISE:
		case TypeLexeme::PLUS_EGAL:
		case TypeLexeme::MOINS_EGAL:
		case TypeLexeme::DIVISE_EGAL:
		case TypeLexeme::MULTIPLIE_EGAL:
		case TypeLexeme::MODULO_EGAL:
		case TypeLexeme::ET_EGAL:
		case TypeLexeme::OU_EGAL:
		case TypeLexeme::OUX_EGAL:
		case TypeLexeme::DEC_DROITE_EGAL:
		case TypeLexeme::DEC_GAUCHE_EGAL:
		case TypeLexeme::ESPERLUETTE:
		case TypeLexeme::POURCENT:
		case TypeLexeme::INFERIEUR:
		case TypeLexeme::INFERIEUR_EGAL:
		case TypeLexeme::SUPERIEUR:
		case TypeLexeme::SUPERIEUR_EGAL:
		case TypeLexeme::DECALAGE_DROITE:
		case TypeLexeme::DECALAGE_GAUCHE:
		case TypeLexeme::DIFFERENCE:
		case TypeLexeme::ESP_ESP:
		case TypeLexeme::EGALITE:
		case TypeLexeme::BARRE_BARRE:
		case TypeLexeme::BARRE:
		case TypeLexeme::CHAPEAU:
		case TypeLexeme::POINT:
		case TypeLexeme::EGAL:
		case TypeLexeme::TROIS_POINTS:
		case TypeLexeme::VIRGULE:
		case TypeLexeme::CROCHET_OUVRANT:
		case TypeLexeme::DECLARATION_VARIABLE:
			return true;
		default:
			return false;
	}
}
