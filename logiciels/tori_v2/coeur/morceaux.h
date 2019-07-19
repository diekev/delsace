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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace langage {

enum {
	IDENTIFIANT_IMPRIME,
	IDENTIFIANT_EXPRIME,
	IDENTIFIANT_CHRONOMETRE,
	IDENTIFIANT_TEMPS,
	IDENTIFIANT_FONCTION,
	IDENTIFIANT_RETOURNE,

	/* caractères */
	IDENTIFIANT_PARENTHESE_OUVERTE,
	IDENTIFIANT_PARENTHESE_FERMEE,
	IDENTIFIANT_ACCOLADE_OUVERTE,
	IDENTIFIANT_ACCOLADE_FERMEE,
	IDENTIFIANT_EGAL,
	IDENTIFIANT_CHAINE_CARACTERE,
	IDENTIFIANT_POINT_VIRGULE,
	IDENTIFIANT_CROCHET_OUVERT,
	IDENTIFIANT_CROCHET_FERME,
	IDENTIFIANT_VIRGULE,
	IDENTIFIANT_NOMBRE,
	IDENTIFIANT_GUILLEMET,
	IDENTIFIANT_NOUVELLE_LIGNE,
	IDENTIFIANT_DOUBLE_POINT,

	/* arithmétique */
	IDENTIFIANT_ADDITION,
	IDENTIFIANT_SOUSTRACTION,
	IDENTIFIANT_DIVISION,
	IDENTIFIANT_MULTIPLICATION,
	IDENTIFIANT_NON,
	IDENTIFIANT_ET,
	IDENTIFIANT_OU,
	IDENTIFIANT_OUX,

	/* comparaison */
	IDENTIFIANT_EGALITE,
	IDENTIFIANT_INEGALITE,
	IDENTIFIANT_INFERIEUR,
	IDENTIFIANT_INFERIEUR_EGAL,
	IDENTIFIANT_SUPERIEUR,
	IDENTIFIANT_SUPERIEUR_EGAL,
	IDENTIFIANT_VRAI,
	IDENTIFIANT_FAUX,

	IDENTIFIANT_NUL,
};

struct DonneesMorceaux {
	static constexpr int INCONNU = IDENTIFIANT_NUL;
	using type = int;

	int identifiant = 0;
	int numero_ligne = 0;
	int position_ligne = 0;
	dls::chaine contenu = "";
	dls::vue_chaine ligne;

	DonneesMorceaux() = default;
};

}  /* namespace langage */
