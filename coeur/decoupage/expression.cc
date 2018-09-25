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

#include "morceaux.h"

enum {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	int direction;
	int priorite;
};

static DonneesPrecedence associativite(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_BARE_BARRE:
			return { GAUCHE, 0 };
		case IDENTIFIANT_ESP_ESP:
			return { GAUCHE, 1 };
		case IDENTIFIANT_BARRE:
			return { GAUCHE, 2};
		case IDENTIFIANT_CHAPEAU:
			return { GAUCHE, 3};
		case IDENTIFIANT_ESPERLUETTE:
			return { GAUCHE, 4};
		case IDENTIFIANT_DIFFERENCE:
		case IDENTIFIANT_EGALITE:
			return { GAUCHE, 5};
		case IDENTIFIANT_INFERIEUR:
		case IDENTIFIANT_INFERIEUR_EGAL:
		case IDENTIFIANT_SUPERIEUR:
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return { GAUCHE, 6};
		case IDENTIFIANT_DECALAGE_GAUCHE:
		case IDENTIFIANT_DECALAGE_DROITE:
			return { GAUCHE, 7};
		case IDENTIFIANT_PLUS:
		case IDENTIFIANT_MOINS:
			return { GAUCHE, 8};
		case IDENTIFIANT_FOIS:
		case IDENTIFIANT_DIVISE:
		case IDENTIFIANT_POURCENT:
			return { GAUCHE, 9};
		case IDENTIFIANT_EXCLAMATION:
			return { GAUCHE, 10 };
	}

	return { GAUCHE, 0 };
}

bool precedence_faible(int identifiant1, int identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == DROITE) && (p1.priorite < p2.priorite));
}
