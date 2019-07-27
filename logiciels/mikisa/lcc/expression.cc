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

#include "expression.h"

#include <cassert>

enum class dir_associativite : int {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	dir_associativite direction;
	int priorite;
};

static DonneesPrecedence associativite(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::TROIS_POINTS:
			return { dir_associativite::GAUCHE, 0 };
		case id_morceau::EGAL:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MOINS_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::FOIS_EGAL:
			return { dir_associativite::GAUCHE, 1 };
		case id_morceau::VIRGULE:
			return { dir_associativite::GAUCHE, 2 };
		case id_morceau::BARRE_BARRE:
			return { dir_associativite::GAUCHE, 3 };
		case id_morceau::ESP_ESP:
			return { dir_associativite::GAUCHE, 4 };
		case id_morceau::BARRE:
			return { dir_associativite::GAUCHE, 5 };
		case id_morceau::CHAPEAU:
			return { dir_associativite::GAUCHE, 6 };
		case id_morceau::ESPERLUETTE:
			return { dir_associativite::GAUCHE, 7 };
		case id_morceau::DIFFERENCE:
		case id_morceau::EGALITE:
			return { dir_associativite::GAUCHE, 8 };
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
			return { dir_associativite::GAUCHE, 9 };
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DECALAGE_DROITE:
			return { dir_associativite::GAUCHE, 10 };
		case id_morceau::PLUS:
		case id_morceau::MOINS:
			return { dir_associativite::GAUCHE, 11 };
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::POURCENT:
			return { dir_associativite::GAUCHE, 12 };
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::AROBASE:
		case id_morceau::DOLLAR:
		case id_morceau::DE:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return { dir_associativite::DROITE, 13 };
		case id_morceau::POINT:
		case id_morceau::CROCHET_OUVRANT:
			return { dir_associativite::GAUCHE, 14 };
		default:
			assert(false);
			return { static_cast<dir_associativite>(-1), -1 };
	}
}

bool precedence_faible(id_morceau identifiant1, id_morceau identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == dir_associativite::GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == dir_associativite::DROITE) && (p1.priorite < p2.priorite));
}
