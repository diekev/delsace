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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cmath>

#include "../math/outils.hh"

namespace dls {
namespace nombre_decimaux {

/**
 * Quantify floating-point numbers in the range [-0.5, 0.5] as 8 or 16-bits
 * integers, to save space for caching.
 *
 * The quantization of x to 8-bit is done with the following formula:
 *
 * char c = round(x * 2^8 + 0.5);
 *
 * to convert back to floating point:
 *
 * float x = float(c) * (1 / 2^8);
 *
 * 8-bit quantization remains correct up to 2 decimal places whilst 16-bit
 * remains correct up to 4 decimal places.
 */

namespace internal {

template <size_t size>
struct quantify_helper;

template <>
struct quantify_helper<8> {
	using type = char;
};

template <>
struct quantify_helper<16> {
	using type = short;
};

}  /* namespace internal */

/**
 * convert the given floating-point number using bit-quantization
 */
template <size_t size, typename real>
auto quantify(real x) -> typename internal::quantify_helper<size>::type
{
	using type = typename internal::quantify_helper<size>::type;

	if (x == real(0)) {
		return type(0);
	}

	return type(round(x * real(math::puissance<2, size>::valeur) + real(0.5)));
}

/**
 * convert the given bit quantified number back to a floating-point number
 */
template <typename real, typename T>
auto dequantify(T x) -> real
{
	if (x == 0) {
		return real(0);
	}

	const auto size = math::nombre_bit<T>::valeur;
	return real(x) * math::reciproque(real(math::puissance<2, size>::valeur));
}

}  /* namespace nombre_decimaux */
}  /* namespace dls */
