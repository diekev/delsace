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

#include "biblinternes/math/vecteur.hh"

namespace dls::phys {

template <typename T>
struct rayon {
	math::point3<T> origine{};
	math::vec3<T> direction{};
	math::vec3<T> direction_inverse{};

	T distance_min = 0;
	T distance_max = 0;

	/* Pour les moteurs de rendu afin d'entrepoler les flous de mouvement. */
	T temps = 0;
};

template <typename T>
struct entresection {
	math::point3<T> point{};

	/* Un peu redondant avec point au dessus, mais peut tout de même être utile */
	T distance = 0;

	/* Utilisé principalement pour définir le triangle ou autre primitive ayant
	 * été entresecté. */
	long idx = 0;

	/* Peut être utilisé par les moteurs de rendu pour pointer sur le bon objet */
	long idx_objet = 0;

	/* Peut être utilisé par les moteurs de rendu pour discriminer le type de
	 * l'objet ayant été entresecté. */
	int type = 0;

	bool touche = false;
};

using rayonf = rayon<float>;
using rayond = rayon<double>;

using esectf = entresection<float>;
using esectd = entresection<double>;

/**
 * Performe un test d'entresection rapide entre le rayon et les bornes d'une
 * boite englobante.
 *
 * Algorithme issu de
 * https://tavianator.com/fast-branchless-raybounding-box-entresections-part-2-nans/
 */
template <typename T>
auto entresection_rapide_min_max(
		rayon<T> const &r,
		math::point3<T> const &min,
		math::point3<T> const &max)
{
	if (r.origine >= min && r.origine <= max) {
		return static_cast<T>(0.0);
	}

	auto t1 = (min[0] - r.origine[0]) * r.direction_inverse[0];
	auto t2 = (max[0] - r.origine[0]) * r.direction_inverse[0];

	auto tmin = std::min(t1, t2);
	auto tmax = std::max(t1, t2);

	for (size_t i = 1; i < 3; ++i) {
		t1 = (min[i] - r.origine[i]) * r.direction_inverse[i];
		t2 = (max[i] - r.origine[i]) * r.direction_inverse[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	/* pour retourner une valeur booléenne : return tmax > std::max(tmin, 0.0); */

	if (tmax < static_cast<T>(0.0) || tmin > tmax) {
		return static_cast<T>(-1.0);
	}

	return tmin;
}

}  /* namespace dls::phys */
