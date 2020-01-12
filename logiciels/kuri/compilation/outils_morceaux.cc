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

#include "outils_morceaux.hh"

#include "morceaux.hh"

bool est_type_entier(id_morceau type)
{
	switch (type) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
		case id_morceau::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_naturel(id_morceau type)
{
	switch (type) {
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_relatif(id_morceau type)
{
	switch (type) {
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
			return true;
		default:
			return false;
	}
}

bool est_type_reel(id_morceau type)
{
	switch (type) {
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::R128:
			return true;
		default:
			return false;
	}
}

bool est_operateur_bool(id_morceau type)
{
	switch (type) {
		case id_morceau::EXCLAMATION:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool est_assignation_operee(id_morceau type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case id_morceau::MOINS_EGAL:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::DEC_DROITE_EGAL:
		case id_morceau::DEC_GAUCHE_EGAL:
		{
			return true;
		}
	}
}

id_morceau operateur_pour_assignation_operee(id_morceau type)
{
	switch (type) {
		default:
		{
			return type;
		}
		case id_morceau::MOINS_EGAL:
		{
			return id_morceau::MOINS;
		}
		case id_morceau::PLUS_EGAL:
		{
			return id_morceau::PLUS;
		}
		case id_morceau::MULTIPLIE_EGAL:
		{
			return id_morceau::FOIS;
		}
		case id_morceau::DIVISE_EGAL:
		{
			return id_morceau::DIVISE;
		}
		case id_morceau::MODULO_EGAL:
		{
			return id_morceau::POURCENT;
		}
		case id_morceau::ET_EGAL:
		{
			return id_morceau::ESPERLUETTE;
		}
		case id_morceau::OU_EGAL:
		{
			return id_morceau::BARRE;
		}
		case id_morceau::OUX_EGAL:
		{
			return id_morceau::CHAPEAU;
		}
		case id_morceau::DEC_DROITE_EGAL:
		{
			return id_morceau::DECALAGE_DROITE;
		}
		case id_morceau::DEC_GAUCHE_EGAL:
		{
			return id_morceau::DECALAGE_GAUCHE;
		}
	}
}

bool est_operateur_comp(id_morceau type)
{
	switch (type) {
		default:
		{
			return false;
		}
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::EGALITE:
		case id_morceau::DIFFERENCE:
		{
			return true;
		}
	}
}

bool peut_etre_dereference(id_morceau id)
{
	switch (id & 0xff) {
		default:
			return false;
		case id_morceau::POINTEUR:
		case id_morceau::REFERENCE:
		case id_morceau::TABLEAU:
		case id_morceau::TROIS_POINTS:
			return true;
	}
}

bool est_mot_cle(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::FONC:
		case id_morceau::STRUCT:
		case id_morceau::DYN:
		case id_morceau::SOIT:
		case id_morceau::RETOURNE:
		case id_morceau::ENUM:
		case id_morceau::RETIENS:
		case id_morceau::DE:
		case id_morceau::EXTERNE:
		case id_morceau::IMPORTE:
		case id_morceau::POUR:
		case id_morceau::DANS:
		case id_morceau::BOUCLE:
		case id_morceau::TANTQUE:
		case id_morceau::REPETE:
		case id_morceau::SINON:
		case id_morceau::SI:
		case id_morceau::SAUFSI:
		case id_morceau::LOGE:
		case id_morceau::DELOGE:
		case id_morceau::RELOGE:
		case id_morceau::DISCR:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::R128:
		case id_morceau::EINI:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::CHAINE:
		case id_morceau::OCTET:
		case id_morceau::UNION:
		case id_morceau::COROUT:
		case id_morceau::CHARGE:
		{
			return true;
		}
	}
}

bool est_chaine_litterale(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::CHAINE_LITTERALE:
		case id_morceau::CARACTERE:
		{
			return true;
		}
	}
}

bool est_specifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::FOIS:
		case id_morceau::ESPERLUETTE:
		case id_morceau::CROCHET_OUVRANT:
		case id_morceau::TROIS_POINTS:
		case id_morceau::TYPE_DE:
			return true;
		default:
			return false;
	}
}

bool est_identifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::R128:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::EINI:
		case id_morceau::CHAINE:
		case id_morceau::CHAINE_CARACTERE:
		case id_morceau::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_nombre_entier(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::NOMBRE_BINAIRE:
		case id_morceau::NOMBRE_ENTIER:
		case id_morceau::NOMBRE_HEXADECIMAL:
		case id_morceau::NOMBRE_OCTAL:
			return true;
		default:
			return false;
	}
}

bool est_nombre(id_morceau identifiant)
{
	return est_nombre_entier(identifiant) || (identifiant == id_morceau::NOMBRE_REEL);
}

bool est_operateur_unaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::AROBASE:
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::CROCHET_OUVRANT:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}

bool est_operateur_binaire(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MOINS_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::DEC_DROITE_EGAL:
		case id_morceau::DEC_GAUCHE_EGAL:
		case id_morceau::ESPERLUETTE:
		case id_morceau::POURCENT:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
		case id_morceau::DE:
		case id_morceau::POINT:
		case id_morceau::EGAL:
		case id_morceau::TROIS_POINTS:
		case id_morceau::VIRGULE:
		case id_morceau::CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}
