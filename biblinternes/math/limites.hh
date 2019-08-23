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

#include "vecteur.hh"

/* Généralisation d'une boîte englobante 3D, pour définir des limites pour
 * différents types, différentes dimensions. */
template <typename T>
struct limites {
	T min{};
	T max{};

	limites() = default;

	limites(T const &init_min, T const &init_max) noexcept
		: min(init_min)
		, max(init_max)
	{}

	T taille() const noexcept
	{
		return max - min;
	}

	void etends(T const &delta) noexcept
	{
		this->min -= delta;
		this->max += delta;
	}

	bool chevauchent(limites const &autres) const noexcept
	{
		return (this->max >= autres.min) && (this->min <= autres.max);
	}

	bool contient(T const &p) const noexcept
	{
		return min <= p && p <= max;
	}
};

using limites2f = limites<dls::math::vec2f>;
using limites2i = limites<dls::math::vec2i>;
using limites3f = limites<dls::math::vec3f>;
using limites3i = limites<dls::math::vec3i>;

namespace dls::math {

template <typename T>
inline auto hors_limites(T const &valeur, T const &min, T const &max)
{
	return valeur < min || valeur > max;
}

template <typename T>
inline auto hors_limites(vec3<T> const &valeur, vec3<T> const &min, vec3<T> const &max)
{
	return valeur < min || valeur > max;
}

template <typename T>
inline auto hors_limites(T const &valeur, limites<T> const &lmt)
{
	return hors_limites(valeur, lmt.min, lmt.max);
}

}  /* namespace dls::math */

