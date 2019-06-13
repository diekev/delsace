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

#include "concepts.hh"
#include "outils.hh"

namespace dls::math {

/* https://en.wikipedia.org/wiki/Smoothstep */
/* Retourne une interpolation fluide Hermite entre 0 et 1. */
template <int N, ConceptDecimal T>
[[nodiscard]] constexpr auto entrepolation_fluide(T x) noexcept
{
	if (x <= static_cast<T>(0)) {
		return static_cast<T>(0);
	}

	if (x > static_cast<T>(1)) {
		return static_cast<T>(1);
	}

	/* 1er-ordre : x */
	if constexpr (N == 0) {
		return x;
	}
	/* 3e-ordre : -2x^3 + 3x^2 */
	else if constexpr (N == 1) {
		return (x * x * (3 - 2 * x));
	}
	/* 5e-ordre : 6x^5 - 15x^4 + 10x^3 */
	else if constexpr (N == 2) {
		return (x * x * x * (x * (x * 6 - 15) + 10));
	}
	/* 7e-ordre : -20x^7 + 70x^6 - 84x^5 + 35x^4 */
	else if constexpr (N == 3) {
		return (x * x * x * x * (x * (x * (x * -20 + 70) - 84) + 35));
	}
	/* 9e-ordre : 70x^9 - 315x^8 + 540x^7 - 420x^6 + 126x^5 */
	else if constexpr (N == 4) {
		return (x * x * x * x * x * (x * (x * (x * (x * 70 - 315) + 540) - 420) + 126));
	}
	/* 11e-ordre : -252x^11 + 1386x^10 - 3080x^9 + 3465x^8 - 1980x^7 + 462x^6 */
	else if constexpr (N == 5) {
		return (x * x * x * x * x * x * (x * (x * (x * (x * (x * -252 + 1386) - 3080) + 3465) - 1980) + 462));
	}
	/* 13e-ordre : 924x^13 - 6006x^12 + 16380x^11 - 24024x^10 + 20020x^9 - 9009x^8 + 1716x^7 */
	else if constexpr (N == 6) {
		return (x * x * x * x * x * x * x * (x * (x * (x * (x * (x * (x * 924 - 6006) + 16380) - 24024) + 20020) - 9009) + 1716));
	}
	else {
		static_assert(N < 7, "interp_fluide : l'ordre de la polynomiale est trop grand");
	}
}

/* Retourne une interpolation fluide Hermite entre 0 et 1, si le _X se situe dans la plage [_Min, _Max] */
template <int N, ConceptDecimal T>
[[nodiscard]] constexpr auto entrepolation_fluide(T x, T min, T max) noexcept
{
	x = restreint((x - min) / (max - min), static_cast<float>(0), static_cast<float>(1));
	return entrepolation_fluide<N>(x);
}

template <int N, ConceptDecimal T>
[[nodiscard]] constexpr auto entrepolation_fluide(T n, T n_bas, T n_haut, T valeur_basse, T valeur_haute) noexcept
{
	return valeur_basse + entrepolation_fluide<N>(n, n_bas, n_haut) * (valeur_haute - valeur_basse);
}

template <int N, ConceptDecimal T>
[[nodiscard]] constexpr auto rampe(T n) noexcept
{
	return entrepolation_fluide<N>((n + static_cast<T>(1)) * static_cast<T>(0.5)) * static_cast<T>(2) - static_cast<T>(1);
}

template <ConceptNombre T>
[[nodiscard]] inline auto entrepolation_lineaire(const T &min, const T &max, const T &f)
{
	return (static_cast<T>(1) - f) * min + f * max;
}

template <ConceptNombre T>
[[nodiscard]] inline auto entrepolation_bilineaire(
		const T &v00, const T &v10,
		const T &v01, const T &v11,
		T fx, T fy) noexcept
{
	return entrepolation_lineaire(
				entrepolation_lineaire(v00, v10, fx),
				entrepolation_lineaire(v01, v11, fx),
				fy);
}

template <ConceptNombre T>
[[nodiscard]] inline auto entrepolation_trilineaire(
		const T &v000, const T &v100,
		const T &v010, const T &v110,
		const T &v001, const T &v101,
		const T &v011, const T &v111,
		T fx, T fy, T fz) noexcept
{
	return entrepolation_lineaire(
				entrepolation_bilineaire(v000, v100, v010, v110, fx, fy),
				entrepolation_bilineaire(v001, v101, v011, v111, fx, fy),
				fz);
}

template <class S, class T>
[[nodiscard]] inline auto entrepolation_quadrilineaire(
		const T &v0000, const T &v1000,
		const T &v0100, const T &v1100,
		const T &v0010, const T &v1010,
		const T &v0110, const T &v1110,
		const T &v0001, const T &v1001,
		const T &v0101, const T &v1101,
		const T &v0011, const T &v1011,
		const T &v0111, const T &v1111,
		T fx, T fy, T fz, T ft) noexcept
{
	return entrepolation_lineaire(
				entrepolation_trilineaire(v0000, v1000, v0100, v1100, v0010, v1010, v0110, v1110, fx, fy, fz),
				entrepolation_trilineaire(v0001, v1001, v0101, v1101, v0011, v1011, v0111, v1111, fx, fy, fz),
				ft);
}

}  /* namespace dls::math */
