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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

namespace dls {
namespace math {

/**
 * Struct créée pour imposer un typage stricte.
 */
struct Hauteur {
	int valeur = 0;

	Hauteur() = default;

	explicit Hauteur(int v);
};

/**
 * Struct créée pour imposer un typage stricte.
 */
struct Largeur {
	int valeur = 0;

	Largeur() = default;

	explicit Largeur(int v);
};

/**
 * Struct créée pour imposer un typage stricte.
 */
struct Profondeur {
	int valeur = 0;

	Profondeur() = default;

	explicit Profondeur(int v);
};

/**
 * Struct créée pour imposer un typage stricte.
 */
struct Dimensions {
	int hauteur = 0;
	int largeur = 0;
	int profondeur = 0;

	Dimensions() = default;

	Dimensions(const Dimensions &autre);

	Dimensions(Hauteur h, Largeur l);

	Dimensions(Hauteur h, Largeur l, Profondeur p);

	Dimensions &operator=(const Dimensions &autre);

	int nombre_elements() const;
};

inline bool operator==(const Dimensions &a, const Dimensions &b)
{
	return a.hauteur == b.hauteur
			&& a.largeur == b.largeur
			&& a.profondeur == b.profondeur;
}

inline bool operator!=(const Dimensions &a, const Dimensions &b)
{
	return !(a == b);
}

}  /* namespace math */
}  /* namespace dls */
