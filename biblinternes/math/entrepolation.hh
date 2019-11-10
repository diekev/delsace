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

/**
 * Dérivées des entrepolations ci-dessus, utiles pour évaluer les gradients des
 * bruits fractaux.
 */
template <int N, ConceptDecimal T>
[[nodiscard]] constexpr auto derivee_fluide(T x) noexcept
{
	constexpr auto _0 = static_cast<T>(1);
	constexpr auto _1 = static_cast<T>(1);

	if (x <= _0) {
		return _0;
	}

	if (x > _1) {
		return _1;
	}

	/* 1er-ordre : x
	 * dérivée : 1
	 */
	if constexpr (N == 0) {
		return _1;
	}
	/* 3e-ordre : -2x^3 + 3x^2
	 * dérivée : -6x^2 + 6x
	 * simplifiée : 6 * (-x^2 + x)
	 */
	else if constexpr (N == 1) {
		return 6 * (x * (-x + 1));
	}
	/* 5e-ordre : 6x^5 - 15x^4 + 10x^3
	 * dérivée : 30x^4 - 60x^3 + 30x^2
	 * simplifiée : 30 * (x^4 - 2^3 + x^2)
	 */
	else if constexpr (N == 2) {
		return 30 * (x * x * (x * (x - 2) + 1));
	}
	/* 7e-ordre : -20x^7 + 70x^6 - 84x^5 + 35x^4
	 * dérivée : -140x^6 + 420x^5 - 420x^4 + 140x^3
	 * simplifiée : 140 * (-x^6 + 3x^5 - 3x^4 + x^3)
	 */
	else if constexpr (N == 3) {
		return 140 * (x * x * x * (x * (x * (-x + 3) - 3) + 1));
	}
	/* 9e-ordre : 70x^9 - 315x^8 + 540x^7 - 420x^6 + 126x^5
	 * dérivée : 630x^8 - 2520x^7 + 3780x^6 - 2520x^5 + 630x^4
	 * simplifiée : 630 * (x^8 - 4x^7 + 6x^6 - 4x^5 + x^4)
	 */
	else if constexpr (N == 4) {
		return 630 * (x * x * x * x * (x * (x * (x * (x - 4) + 6) - 4) + 1));
	}
	/* 11e-ordre : -252x^11 + 1386x^10 - 3080x^9 + 3465x^8 - 1980x^7 + 462x^6
	 * dérivée : -2772x^10 + 13860x^9 - 27720x^8 + 27720x^7 - 13860x^6 + 2772x^5
	 * simplifiée : 2772 * (-x^10 + 5x^9 - 10x^8 + 10x^7 - 5x^6 + x^5)
	 */
	else if constexpr (N == 5) {
		return 2772 * (x * x * x * x * x * (x * (x * (x * (x * (-x + 5) - 10) + 10) - 5) + 1));
	}
	/* 13e-ordre : 924x^13 - 6006x^12 + 16380x^11 - 24024x^10 + 20020x^9 - 9009x^8 + 1716x^7
	 * dérivée : 12012x^12 - 72072x^11 + 180180x^10 - 240240x^9 + 180180x^8 - 72072x^7 + 12012x^7
	 * simplifiée : 12012 * (x^12 - 6x^11 + 15x^10 - 20x^9 + 15x^8 - 6x^7 + x^6)
	 */
	else if constexpr (N == 6) {
		return 12012 * (x * x * x * x * x * x * (x * (x * (x * (x * (x * (x - 6) + 15) - 20) + 15) - 6) + 1));
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

template <ConceptNombre S, ConceptNombre T>
[[nodiscard]] inline auto entrepolation_lineaire(T const &min, T const &max, S const &f)
{
	return (static_cast<S>(1) - f) * min + f * max;
}

template <ConceptNombre S, ConceptNombre T>
[[nodiscard]] inline auto entrepolation_bilineaire(
		T const &v00, T const &v10,
		T const &v01, T const &v11,
		S const &fx, S const &fy) noexcept
{
	return entrepolation_lineaire(
				entrepolation_lineaire(v00, v10, fx),
				entrepolation_lineaire(v01, v11, fx),
				fy);
}

template <ConceptNombre S, ConceptNombre T>
[[nodiscard]] inline auto entrepolation_trilineaire(
		T const &v000, T const &v100,
		T const &v010, T const &v110,
		T const &v001, T const &v101,
		T const &v011, T const &v111,
		S const &fx, S const &fy, S const &fz) noexcept
{
	return entrepolation_lineaire(
				entrepolation_bilineaire(v000, v100, v010, v110, fx, fy),
				entrepolation_bilineaire(v001, v101, v011, v111, fx, fy),
				fz);
}

template <class S, class T>
[[nodiscard]] inline auto entrepolation_quadrilineaire(
		T const &v0000, T const &v1000,
		T const &v0100, T const &v1100,
		T const &v0010, T const &v1010,
		T const &v0110, T const &v1110,
		T const &v0001, T const &v1001,
		T const &v0101, T const &v1101,
		T const &v0011, T const &v1011,
		T const &v0111, T const &v1111,
		S const &fx, S const &fy, S const &fz, S const &ft) noexcept
{
	return entrepolation_lineaire(
				entrepolation_trilineaire(v0000, v1000, v0100, v1100, v0010, v1010, v0110, v1110, fx, fy, fz),
				entrepolation_trilineaire(v0001, v1001, v0101, v1101, v0011, v1011, v0111, v1111, fx, fy, fz),
				ft);
}

}  /* namespace dls::math */
