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

#include <iostream>

#include "biblinternes/outils/definitions.h"

enum type_evenement : int {
	/* Category. */
	arbre            = (1 << 0),
	rafraichissement = (2 << 0),

	/* Action. */
	modifie = (1 << 8),
};

DEFINIE_OPERATEURS_DRAPEAU(type_evenement, int)

constexpr auto action_evenement(type_evenement evenement)
{
	return evenement & 0x0000ff00;
}

constexpr auto categorie_evenement(type_evenement evenement)
{
	return evenement & 0x000000ff;
}

template <typename TypeChar>
std::basic_ostream<TypeChar> &operator<<(
		std::basic_ostream<TypeChar> &os,
		type_evenement evenement)
{
	switch (categorie_evenement(evenement)) {
		case type_evenement::arbre:
			os << "dessin, ";
			break;
		case type_evenement::rafraichissement:
			os << "rafraichissement, ";
			break;
		default:
			os << "inconnu, ";
			break;
	}

	switch (action_evenement(evenement)) {
		case type_evenement::modifie:
			os << "modifié";
			break;
		default:
			os << "inconnu";
			break;
	}

	return os;
}
