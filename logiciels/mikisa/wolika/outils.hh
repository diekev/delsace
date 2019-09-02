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

#pragma once

#include "grille_dense.hh"

namespace wlk {

template <typename T>
auto est_vide(wlk::grille_dense_3d<T> const &grille)
{
	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		if (grille.valeur(i) != T(0)) {
			return false;
		}
	}

	return true;
}

template <typename T>
auto remplis_grille(wlk::grille_dense_3d<T> &grille, T valeur)
{
	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		grille.valeur(i) = valeur;
	}
}

template <typename T>
auto extrait_min(grille_dense_2d<T> const &grille)
{
	auto min = std::numeric_limits<T>::max();

	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		auto v = grille.valeur(i);

		if (v < min) {
			min = v;
		}
	}

	return min;
}

template <typename T>
auto extrait_max(grille_dense_2d<T> const &grille)
{
	auto max = -std::numeric_limits<T>::max();

	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		auto v = grille.valeur(i);

		if (v > max) {
			max = v;
		}
	}

	return max;
}

template <typename T>
auto extrait_min_max(grille_dense_2d<T> const &grille, T &min, T &max)
{
	min = std::numeric_limits<T>::max();
	max = -std::numeric_limits<T>::max();

	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		auto v = grille.valeur(i);

		if (v < min) {
			min = v;
		}

		if (v > max) {
			max = v;
		}
	}
}

}  /* namespace wlk */
