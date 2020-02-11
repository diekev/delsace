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

bool est_type_entier(GenreLexeme type)
{
	switch (type) {
		case GenreLexeme::BOOL:
		case GenreLexeme::N8:
		case GenreLexeme::N16:
		case GenreLexeme::N32:
		case GenreLexeme::N64:
		case GenreLexeme::Z8:
		case GenreLexeme::Z16:
		case GenreLexeme::Z32:
		case GenreLexeme::Z64:
		case GenreLexeme::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_naturel(GenreLexeme type)
{
	switch (type) {
		case GenreLexeme::N8:
		case GenreLexeme::N16:
		case GenreLexeme::N32:
		case GenreLexeme::N64:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_relatif(GenreLexeme type)
{
	switch (type) {
		case GenreLexeme::Z8:
		case GenreLexeme::Z16:
		case GenreLexeme::Z32:
		case GenreLexeme::Z64:
			return true;
		default:
			return false;
	}
}

bool est_type_reel(GenreLexeme type)
{
	switch (type) {
		case GenreLexeme::R16:
		case GenreLexeme::R32:
		case GenreLexeme::R64:
			return true;
		default:
			return false;
	}
}

bool est_operateur_bool(GenreLexeme type)
{
	switch (type) {
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::EGALITE:
		case GenreLexeme::BARRE_BARRE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool est_assignation_operee(GenreLexeme type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		{
			return true;
		}
	}
}

GenreLexeme operateur_pour_assignation_operee(GenreLexeme type)
{
	switch (type) {
		default:
		{
			return type;
		}
		case GenreLexeme::MOINS_EGAL:
		{
			return GenreLexeme::MOINS;
		}
		case GenreLexeme::PLUS_EGAL:
		{
			return GenreLexeme::PLUS;
		}
		case GenreLexeme::MULTIPLIE_EGAL:
		{
			return GenreLexeme::FOIS;
		}
		case GenreLexeme::DIVISE_EGAL:
		{
			return GenreLexeme::DIVISE;
		}
		case GenreLexeme::MODULO_EGAL:
		{
			return GenreLexeme::POURCENT;
		}
		case GenreLexeme::ET_EGAL:
		{
			return GenreLexeme::ESPERLUETTE;
		}
		case GenreLexeme::OU_EGAL:
		{
			return GenreLexeme::BARRE;
		}
		case GenreLexeme::OUX_EGAL:
		{
			return GenreLexeme::CHAPEAU;
		}
		case GenreLexeme::DEC_DROITE_EGAL:
		{
			return GenreLexeme::DECALAGE_DROITE;
		}
		case GenreLexeme::DEC_GAUCHE_EGAL:
		{
			return GenreLexeme::DECALAGE_GAUCHE;
		}
	}
}

bool est_operateur_comp(GenreLexeme type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::EGALITE:
		case GenreLexeme::DIFFERENCE:
		{
			return true;
		}
	}
}

bool peut_etre_dereference(GenreLexeme id)
{
	switch (id & 0xff) {
		default:
			return false;
		case GenreLexeme::POINTEUR:
		case GenreLexeme::REFERENCE:
		case GenreLexeme::TABLEAU:
		case GenreLexeme::TROIS_POINTS:
			return true;
	}
}

bool est_mot_cle(GenreLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case GenreLexeme::FONC:
		case GenreLexeme::STRUCT:
		case GenreLexeme::DYN:
		case GenreLexeme::RETOURNE:
		case GenreLexeme::ENUM:
		case GenreLexeme::ENUM_DRAPEAU:
		case GenreLexeme::RETIENS:
		case GenreLexeme::EXTERNE:
		case GenreLexeme::IMPORTE:
		case GenreLexeme::POUR:
		case GenreLexeme::DANS:
		case GenreLexeme::BOUCLE:
		case GenreLexeme::TANTQUE:
		case GenreLexeme::REPETE:
		case GenreLexeme::SINON:
		case GenreLexeme::SI:
		case GenreLexeme::SAUFSI:
		case GenreLexeme::LOGE:
		case GenreLexeme::DELOGE:
		case GenreLexeme::RELOGE:
		case GenreLexeme::DISCR:
		case GenreLexeme::Z8:
		case GenreLexeme::Z16:
		case GenreLexeme::Z32:
		case GenreLexeme::Z64:
		case GenreLexeme::N8:
		case GenreLexeme::N16:
		case GenreLexeme::N32:
		case GenreLexeme::N64:
		case GenreLexeme::R16:
		case GenreLexeme::R32:
		case GenreLexeme::R64:
		case GenreLexeme::EINI:
		case GenreLexeme::BOOL:
		case GenreLexeme::RIEN:
		case GenreLexeme::CHAINE:
		case GenreLexeme::OCTET:
		case GenreLexeme::UNION:
		case GenreLexeme::COROUT:
		case GenreLexeme::CHARGE:
		{
			return true;
		}
	}
}

bool est_chaine_litterale(GenreLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case GenreLexeme::CHAINE_LITTERALE:
		case GenreLexeme::CARACTERE:
		{
			return true;
		}
	}
}

bool est_specifiant_type(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::FOIS:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::TYPE_DE:
			return true;
		default:
			return false;
	}
}

bool est_identifiant_type(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::N8:
		case GenreLexeme::N16:
		case GenreLexeme::N32:
		case GenreLexeme::N64:
		case GenreLexeme::R16:
		case GenreLexeme::R32:
		case GenreLexeme::R64:
		case GenreLexeme::Z8:
		case GenreLexeme::Z16:
		case GenreLexeme::Z32:
		case GenreLexeme::Z64:
		case GenreLexeme::BOOL:
		case GenreLexeme::RIEN:
		case GenreLexeme::EINI:
		case GenreLexeme::CHAINE:
		case GenreLexeme::CHAINE_CARACTERE:
		case GenreLexeme::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_nombre_entier(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::NOMBRE_BINAIRE:
		case GenreLexeme::NOMBRE_ENTIER:
		case GenreLexeme::NOMBRE_HEXADECIMAL:
		case GenreLexeme::NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

bool est_nombre(GenreLexeme identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == GenreLexeme::NOMBRE_REEL);
}

bool est_operateur_unaire(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::AROBASE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

bool est_operateur_binaire(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::POURCENT:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::EGALITE:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::POINT:
		case GenreLexeme::EGAL:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::VIRGULE:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::DECLARATION_VARIABLE:
			return true;
		default:
			return false;
	}
}
