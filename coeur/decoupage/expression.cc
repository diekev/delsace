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
		case ID_BARRE_BARRE:
			return { GAUCHE, 0 };
		case ID_ESP_ESP:
			return { GAUCHE, 1 };
		case ID_BARRE:
			return { GAUCHE, 2};
		case ID_CHAPEAU:
			return { GAUCHE, 3};
		case ID_ESPERLUETTE:
			return { GAUCHE, 4};
		case ID_DIFFERENCE:
		case ID_EGALITE:
			return { GAUCHE, 5};
		case ID_INFERIEUR:
		case ID_INFERIEUR_EGAL:
		case ID_SUPERIEUR:
		case ID_SUPERIEUR_EGAL:
			return { GAUCHE, 6};
		case ID_DECALAGE_GAUCHE:
		case ID_DECALAGE_DROITE:
			return { GAUCHE, 7};
		case ID_PLUS:
		case ID_MOINS:
			return { GAUCHE, 8};
		case ID_FOIS:
		case ID_DIVISE:
		case ID_POURCENT:
			return { GAUCHE, 9};
		case ID_EXCLAMATION:
		case ID_TILDE:
		case ID_AROBASE:
			return { DROITE, 10 };
		case ID_CROCHET_OUVRANT:
			return { GAUCHE, 11 };
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
