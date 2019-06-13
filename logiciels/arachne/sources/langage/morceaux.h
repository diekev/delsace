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

#pragma once

#include <string>

namespace arachne {

enum {
	IDENTIFIANT_BASE_DE_DONNEES,
	IDENTIFIANT_CHARGE,
	IDENTIFIANT_CREE,
	IDENTIFIANT_ET,
	IDENTIFIANT_FAUX,
	IDENTIFIANT_FICHIER,
	IDENTIFIANT_NUL,
	IDENTIFIANT_OU,
	IDENTIFIANT_RETOURNE,
	IDENTIFIANT_SI,
	IDENTIFIANT_SUPPRIME,
	IDENTIFIANT_TROUVE,
	IDENTIFIANT_UTILISE,
	IDENTIFIANT_VRAI,
	IDENTIFIANT_ECRIT,
	IDENTIFIANT_GUILLEMET,
	IDENTIFIANT_APOSTROPHE,
	IDENTIFIANT_PARENTHESE_OUVRANTE,
	IDENTIFIANT_PARENTHESE_FERMANTE,
	IDENTIFIANT_VIRGULE,
	IDENTIFIANT_POINT,
	IDENTIFIANT_DOUBLE_POINT,
	IDENTIFIANT_POINT_VIRGULE,
	IDENTIFIANT_INFERIEUR,
	IDENTIFIANT_SUPERIEUR,
	IDENTIFIANT_CROCHET_OUVRANT,
	IDENTIFIANT_CROCHET_FERMANT,
	IDENTIFIANT_ACCOLADE_OUVRANTE,
	IDENTIFIANT_ACCOLADE_FERMANTE,
	IDENTIFIANT_DIFFERENCE,
	IDENTIFIANT_INFERIEUR_EGAL,
	IDENTIFIANT_EGALITE,
	IDENTIFIANT_SUPERIEUR_EGAL,
	IDENTIFIANT_CHAINE_CARACTERE,
	IDENTIFIANT_CHAINE_LITTERALE,
	IDENTIFIANT_CARACTERE,
	IDENTIFIANT_NOMBRE,
	IDENTIFIANT_NOMBRE_DECIMAL,
	IDENTIFIANT_BOOL,
};

struct DonneesMorceaux {
	int identifiant = 0;
	int numero_ligne = 0;
	int position_ligne = 0;
	std::string contenu = "";
	std::string ligne;

	DonneesMorceaux() = default;
};

const char *chaine_identifiant(int identifiant);

} /* namespace arachne */
