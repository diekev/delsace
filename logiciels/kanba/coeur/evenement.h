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

enum type_evenement : int {
	/* Category. */
	dessin           = (1 << 0),
	calque           = (2 << 0),
	rafraichissement = (3 << 0),
	projet           = (4 << 0),

	/* Action. */
	ajoute    = (1 << 8),
	selection = (2 << 8),
	fini      = (4 << 8),
	supprime  = (5 << 8),
	charge    = (6 << 8),
};

constexpr type_evenement operator&(type_evenement cote_gauche, type_evenement cote_droit)
{
	return static_cast<type_evenement>(static_cast<int>(cote_gauche) & static_cast<int>(cote_droit));
}

constexpr type_evenement operator&(type_evenement cote_gauche, int cote_droit)
{
	return static_cast<type_evenement>(static_cast<int>(cote_gauche) & cote_droit);
}

constexpr type_evenement operator|(type_evenement cote_gauche, type_evenement cote_droit)
{
	return static_cast<type_evenement>(static_cast<int>(cote_gauche) | static_cast<int>(cote_droit));
}

constexpr type_evenement operator^(type_evenement cote_gauche, type_evenement cote_droit)
{
	return static_cast<type_evenement>(static_cast<int>(cote_gauche) ^ static_cast<int>(cote_droit));
}

constexpr type_evenement operator~(type_evenement cote_gauche)
{
	return static_cast<type_evenement>(~static_cast<int>(cote_gauche));
}

type_evenement &operator|=(type_evenement &cote_gauche, type_evenement cote_droit);
type_evenement &operator&=(type_evenement &cote_gauche, type_evenement cote_droit);
type_evenement &operator^=(type_evenement &cote_gauche, type_evenement cote_droit);

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
		case type_evenement::dessin:
			os << "dessin, ";
			break;
		case type_evenement::calque:
			os << "calque, ";
			break;
		case type_evenement::rafraichissement:
			os << "rafraichissement, ";
			break;
		case type_evenement::projet:
			os << "projet, ";
			break;
		default:
			break;
	}

	switch (action_evenement(evenement)) {
		case type_evenement::ajoute:
			os << "ajoute";
			break;
		case type_evenement::selection:
			os << "selection";
			break;
		case type_evenement::fini:
			os << "fini";
			break;
		case type_evenement::supprime:
			os << "supprime";
			break;
		case type_evenement::charge:
			os << "charge";
			break;
		default:
			break;
	}

	return os;
}
