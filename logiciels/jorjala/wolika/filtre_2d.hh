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

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"

#include "grille_dense.hh"
#include "interruptrice.hh"

namespace wlk {

enum class type_filtre : char {
	BOITE,
	TRIANGULAIRE,
	QUADRATIC,
	CUBIC,
	GAUSSIEN,
	MITCHELL,
	CATROM,
};

template <typename T>
auto valeur_filtre_quadratic(T x)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _0_5 = static_cast<T>(0.5);
	constexpr auto _0_75 = static_cast<T>(0.75);
	constexpr auto _1_5 = static_cast<T>(1.5);

	if (x < _0) {
		x = -x;
	}

	if (x < _0_5) {
		return _0_75 - (x * x);
	}

	if (x < _1_5) {
		return _1_5 * (x - _1_5) * (x - _1_5);
	}

	return _0;
}

template <typename T>
auto valeur_filtre_cubic(T x)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _0_5 = static_cast<T>(0.5);
	constexpr auto _1 = static_cast<T>(1.0);
	constexpr auto _2 = static_cast<T>(2.0);
	constexpr auto _3 = static_cast<T>(3.0);
	constexpr auto _6 = static_cast<T>(6.0);

	auto x2 = x * x;

	if (x < _0) {
		x = -x;
	}

	if (x < _1) {
		return _0_5 * x * x2 - x2 + _2 / _3;
	}

	if (x < _2) {
		return (_2 - x) * (_2 - x) * (_2 - x) / _6;
	}

	return _0;
}

template <typename T>
auto valeur_filtre_catrom(T x)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _0_5 = static_cast<T>(0.5);
	constexpr auto _1 = static_cast<T>(1.0);
	constexpr auto _1_5 = static_cast<T>(1.5);
	constexpr auto _2 = static_cast<T>(2.0);
	constexpr auto _2_5 = static_cast<T>(2.5);
	constexpr auto _4 = static_cast<T>(4.0);

	auto x2 = x * x;

	if (x < _0) {
		x = -x;
	}

	if (x < _1) {
		return _1_5 * x2 * x - _2_5 * x2 + _1;
	}
	if (x < _2) {
		return -_0_5 * x2 * x + _2_5 * x2 - _4 * x + _2;
	}

	return _0;
}

/* bicubic de Mitchell & Netravali */
template <typename T>
auto valeur_filtre_mitchell(T x)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _1 = static_cast<T>(1.0);
	constexpr auto _2 = static_cast<T>(2.0);
	constexpr auto _3 = static_cast<T>(3.0);
	constexpr auto _6 = static_cast<T>(6.0);
	constexpr auto _8 = static_cast<T>(8.0);
	constexpr auto _9 = static_cast<T>(9.0);
	constexpr auto _12 = static_cast<T>(12.0);
	constexpr auto _18 = static_cast<T>(18.0);
	constexpr auto _24 = static_cast<T>(24.0);
	constexpr auto _30 = static_cast<T>(30.0);
	constexpr auto _48 = static_cast<T>(48.0);

	auto const b = _1 / _3;
	auto const c = _1 / _3;
	auto const p0 = (_6 - _2 * b) / _6;
	auto const p2 = (-_18 + _12 * b + _6 * c) / _6;
	auto const p3 = (_12 - _9 * b - _6 * c) / _6;
	auto const q0 = (_8 * b + _24 * c) / _6;
	auto const q1 = (-_12 * b - _48 * c) / _6;
	auto const q2 = (_6 * b + _30 * c) / _6;
	auto const q3 = (-b - _6 * c) / _6;

	if (x < -_2) {
		return 0.0f;
	}

	if (x < -_1) {
		return (q0 - x * (q1 - x * (q2 - x * q3)));
	}

	if (x < _0) {
		return (p0 + x * x * (p2 - x * p3));
	}

	if (x < _1) {
		return (p0 + x * x * (p2 + x * p3));
	}

	if (x < _2) {
		return (q0 + x * (q1 + x * (q2 + x * q3)));
	}

	return _0;
}

/* x va de -1 à 1 */
template <typename T>
auto valeur_filtre(type_filtre type, T x)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _1 = static_cast<T>(1.0);
	constexpr auto _2 = static_cast<T>(2.0);
	constexpr auto _3 = static_cast<T>(3.0);

	auto const gaussfac = static_cast<T>(1.6);

	x = std::abs(x);

	switch (type) {
		case type_filtre::BOITE:
		{
			if (x > _1) {
				return _0;
			}

			return _1;
		}
		case type_filtre::TRIANGULAIRE:
		{
			if (x > _1) {
				return _0;
			}

			return _1 - x;
		}
		case type_filtre::GAUSSIEN:
		{
			const float two_gaussfac2 = _2 * gaussfac * gaussfac;
			x *= _3 * gaussfac;

			return _1 / std::sqrt(constantes<T>::PI * two_gaussfac2) * std::exp(-x * x / two_gaussfac2);
		}
		case type_filtre::MITCHELL:
		{
			return valeur_filtre_mitchell(x * gaussfac);
		}
		case type_filtre::QUADRATIC:
		{
			return valeur_filtre_quadratic(x * gaussfac);
		}
		case type_filtre::CUBIC:
		{
			return valeur_filtre_cubic(x * gaussfac);
		}
		case type_filtre::CATROM:
		{
			return valeur_filtre_catrom(x * gaussfac);
		}
	}

	return _0;
}

template <typename T>
auto cree_table_filtre(type_filtre type, T rayon)
{
	/* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
	 * static_cast partout */
	constexpr auto _0 = static_cast<T>(0);
	constexpr auto _1 = static_cast<T>(1.0);

	auto taille = static_cast<int>(rayon);
	auto n = 2 * taille + 1;

	auto table = memoire::loge_tableau<T>("table_filtre", n);

	auto fac = (rayon > _0 ? _1 / rayon : _0);
	auto somme = _0;

	for (auto i = -taille; i <= taille; i++) {
		auto val = valeur_filtre(type, static_cast<T>(i) * fac);
		somme += val;
		table[i + taille] = val;
	}

	somme = 1.0f / somme;

	for (auto i = 0; i < n; i++) {
		table[i] *= somme;
	}

	return table;
}

template <typename T>
auto detruit_table_filtre(T *ptr, T rayon)
{
	memoire::deloge_tableau("table_filtre", ptr, 2 * static_cast<long>(rayon) + 1);
}

template <typename T>
void copie_donnees_grille(
		grille_dense_2d<T> const &tampon_de,
		grille_dense_2d<T> &tampon_vers)
{
	auto nombre_elements = tampon_de.nombre_elements();

	for (auto i = 0; i < nombre_elements; ++i) {
		tampon_vers.valeur(i) = tampon_de.valeur(i);
	}
}

template <typename TG, typename  T>
auto filtre_grille(
		grille_dense_2d<TG> &grille,
		type_filtre type,
		T rayon,
		interruptrice *chef = nullptr)
{
	auto table = cree_table_filtre(type, rayon);
	auto grille_tmp = grille_couleur(grille.desc());

	auto const res_x = grille.desc().resolution.x;
	auto const res_y = grille.desc().resolution.y;

	/* applique filtre sur l'axe des X */
	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef && chef->interrompue()) {
			return;
		}

		for (int y = plage.begin(); y < plage.end(); ++y) {
			if (chef && chef->interrompue()) {
				return;
			}

			for (int x = 0; x < res_x; ++x) {
				auto index = grille.calcul_index(dls::math::vec2i(x, y));

				auto valeur = TG(0.0);

				for (auto ix = x - static_cast<int>(rayon), k = 0; ix < x + static_cast<int>(rayon) + 1; ix++, ++k) {
					auto const &p = grille.valeur(dls::math::vec2i(ix, y));
					valeur += p * table[k];
				}

				grille_tmp.valeur(index) = valeur;
			}
		}

		if (chef) {
			auto delta = static_cast<float>(plage.end() - plage.begin());
			delta /= static_cast<float>(res_y);
			chef->indique_progression_parallele(delta * 50.0f);
		}
	});

	copie_donnees_grille(grille_tmp, grille);

	/* applique filtre sur l'axe des Y */
	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef && chef->interrompue()) {
			return;
		}

		for (int y = plage.begin(); y < plage.end(); ++y) {
			if (chef && chef->interrompue()) {
				return;
			}

			for (int x = 0; x < res_x; ++x) {
				auto index = grille.calcul_index(dls::math::vec2i(x, y));

				auto valeur = TG(0.0);

				for (auto iy = y - static_cast<int>(rayon), k = 0; iy < y + static_cast<int>(rayon) + 1; iy++, ++k) {
					auto const &p = grille.valeur(dls::math::vec2i(x, iy));
					valeur += p * table[k];
				}

				grille_tmp.valeur(index) = valeur;
			}
		}

		if (chef) {
			auto delta = static_cast<float>(plage.end() - plage.begin());
			delta /= static_cast<float>(res_y);
			chef->indique_progression_parallele(delta * 50.0f);
		}
	});

	copie_donnees_grille(grille_tmp, grille);

	detruit_table_filtre(table, rayon);
}

}  /* namespace wlk */
