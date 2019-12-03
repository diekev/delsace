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

#include "iteration.hh"

namespace wlk {

template <typename T>
struct ajoute {
	T operator()(T const &a, T const &b)
	{
		return a + b;
	}
};

template <typename T>
struct soustrait {
	T operator()(T const &a, T const &b)
	{
		return a - b;
	}
};

template <typename T>
struct multiplie {
	T operator()(T const &a, T const &b)
	{
		return a * b;
	}
};

template <typename T>
struct divise {
	T operator()(T const &a, T const &b)
	{
		return a / b;
	}
};

template <typename T, typename type_tuile, typename Op>
auto fusionne_grilles(
		grille_eparse<T, type_tuile> const &grille_a,
		grille_eparse<T, type_tuile> const &grille_b,
		Op &&op)
{
	auto grille = memoire::loge<wlk::grille_eparse<T>>("wlk::grille_eparse", grille_a.desc());
	grille->assure_tuiles(grille_a.desc().etendue);

	wlk::pour_chaque_tuile_parallele(grille_a, [&](wlk::tuile_scalaire<T> const *tuile_a)
	{
		auto min_tuile = tuile_a->min / wlk::TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(min_tuile, grille_a.res_tuile());
		auto tuile_b = grille_b.tuile_par_index(idx_tuile);
		auto tuile_r = grille->tuile_par_index(idx_tuile);

		for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
			tuile_r->donnees[i] = op(tuile_a->donnees[i], tuile_b->donnees[i]);
		}
	});

	grille->elague();

	return grille;
}

}  /* namespace wlk */
