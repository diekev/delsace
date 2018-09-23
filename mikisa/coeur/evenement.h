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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <iostream>

enum type_evenement : int {
	/* Categorie, 256 entrées. */
	image            = (1 << 0),
	noeud            = (2 << 0),
	temps            = (3 << 0),
	rafraichissement = (4 << 0),
	camera_2d        = (5 << 0),
	camera_3d        = (6 << 0),
	objet            = (7 << 0),
	propriete        = (8 << 0),
	scene            = (9 << 0),

	/* Action, 256 entrées. */
	ajoute      = (1 << 8),
	enleve      = (2 << 8),
	selectionne = (3 << 8),
	modifie     = (4 << 8),
	traite      = (5 << 8),
	manipule    = (6 << 8),
};

constexpr type_evenement operator&(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr type_evenement operator&(type_evenement lhs, int rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) & rhs);
}

constexpr type_evenement operator|(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr type_evenement operator^(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr type_evenement operator~(type_evenement lhs)
{
	return static_cast<type_evenement>(~static_cast<int>(lhs));
}

type_evenement &operator|=(type_evenement &lhs, type_evenement rhs);
type_evenement &operator&=(type_evenement &lhs, type_evenement rhs);
type_evenement &operator^=(type_evenement &lhs, type_evenement rhs);

constexpr auto action_evenement(type_evenement evenement) -> type_evenement
{
	return evenement & 0x0000ff00;
}

constexpr auto categorie_evenement(type_evenement evenement) -> type_evenement
{
	return evenement & 0x000000ff;
}

template <typename char_type>
auto &operator<<(std::basic_ostream<char_type> &os, type_evenement evenement)
{
	switch (categorie_evenement(evenement)) {
		case type_evenement::image:
			os << "image, ";
			break;
		case type_evenement::noeud:
			os << "noeud, ";
			break;
		case type_evenement::temps:
			os << "temps, ";
			break;
		case type_evenement::rafraichissement:
			os << "rafraichissement, ";
			break;
		case type_evenement::camera_2d:
			os << "camera_2d, ";
			break;
		case type_evenement::camera_3d:
			os << "camera_3d, ";
			break;
		case type_evenement::objet:
			os << "objet, ";
			break;
		case type_evenement::propriete:
			os << "propriété, ";
			break;
		case type_evenement::scene:
			os << "scène, ";
			break;
		default:
			os << "inconnu, ";
			break;
	}

	switch (action_evenement(evenement)) {
		case type_evenement::ajoute:
			os << "ajouté";
			break;
		case type_evenement::modifie:
			os << "modifié";
			break;
		case type_evenement::enleve:
			os << "enlevé";
			break;
		case type_evenement::selectionne:
			os << "sélectionné";
			break;
		case type_evenement::traite:
			os << "traité";
			break;
		case type_evenement::manipule:
			os << "manipulé";
			break;
		default:
			os << "inconnu";
			break;
	}

	return os;
}
