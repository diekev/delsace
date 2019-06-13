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

#include <cmath>
#include <limits>

namespace dls::math {

/**
 * Retourne une valeur équivalente à la restriction de la valeur 'a' entre 'min'
 * et 'max'.
 */
template <typename T>
[[nodiscard]] constexpr auto restreint(const T &a, const T &min, const T &max) noexcept
{
	if (a < min) {
		return min;
	}

	if (a > max) {
		return max;
	}

	return a;
}

/**
 * Retourne un nombre entre 'neuf_min' et 'neuf_max' qui est relatif à 'valeur'
 * dans la plage entre 'vieux_min' et 'vieux_max'. Si la valeur est hors de
 * 'vieux_min' et 'vieux_max' il sera restreint à la nouvelle plage.
 */
template <typename T>
[[nodiscard]] constexpr auto traduit(const T valeur, const T vieux_min, const T vieux_max, const T neuf_min, const T neuf_max) noexcept
{
	T tmp;

	if (vieux_min > vieux_max) {
		tmp = vieux_min - restreint(valeur, vieux_max, vieux_min);
	}
	else {
		tmp = restreint(valeur, vieux_min, vieux_max);
	}

	tmp = (tmp - vieux_min) / (vieux_max - vieux_min);
	return neuf_min + tmp * (neuf_max - neuf_min);
}

template <typename T>
[[nodiscard]] constexpr auto carre(T x) noexcept
{
	return x * x;
}

// transforms even the sequence 0,1,2,3,... into reasonably good random numbers
// challenge: improve on this in speed and "randomness"!
[[nodiscard]] inline auto hash_aleatoire(unsigned int seed) noexcept
{
   unsigned int i = (seed ^ 12345391u) * 2654435769u;

   i ^= (i << 6) ^ (i >> 26);
   i *= 2654435769u;
   i += (i << 5) ^ (i >> 12);

   return i;
}

template <typename T>
[[nodiscard]] inline auto hash_aleatoiref(unsigned int seed) noexcept
{
	return static_cast<T>(hash_aleatoire(seed)) / static_cast<T>(std::numeric_limits<unsigned int>::max());
}

template <typename T>
[[nodiscard]] inline auto hash_aleatoiref(unsigned int seed, T a, T b) noexcept
{
	return ((b - a) * hash_aleatoiref<T>(seed) + a);
}

template <typename type_vecteur>
[[nodiscard]] auto echantillone_sphere(unsigned int &seed) noexcept
{
	using type_scalaire = typename type_vecteur::type_scalaire;

	type_vecteur v;
	type_scalaire m2;

	do {
		m2 = static_cast<type_scalaire>(0);

		for (unsigned int i = 0; i < type_vecteur::nombre_composants; ++i) {
			v[i] = hash_aleatoiref(seed++, -static_cast<type_scalaire>(1), static_cast<type_scalaire>(1));
			m2 += carre(v[i]);
		}
	} while (m2 > static_cast<type_scalaire>(1) || m2 == static_cast<type_scalaire>(0));

	return v / std::sqrt(m2);
}

template <typename T>
[[nodiscard]] auto produit_scalaire(T g[3], T x, T y, T z) noexcept
{
	return g[0] * x + g[1] * y + g[2] * z;
}

template <typename T>
[[nodiscard]] auto radians_vers_degrees(T radian) noexcept
{
	return (radian / static_cast<T>(M_PI)) * static_cast<T>(180);
}

template <typename T>
[[nodiscard]] auto degrees_vers_radians(T degre) noexcept
{
	return (degre / static_cast<T>(180)) * static_cast<T>(M_PI);
}

}  /* namespace dls::math */
