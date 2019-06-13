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
#include <random>

namespace dls {
namespace math {

template <typename T, typename URNG>
auto marsaglia(URNG &&gen, const T mean, const T sigma) -> std::pair<T, T>
{
	std::uniform_real_distribution<T> dis(T(-1), T(1));
	T u{0}, v{0}, s{0};

	do {
		u = dis(gen);
		v = dis(gen);
		s = u * u + v * v;
	} while(s >= T(1) || s == T(0));

	s = std::sqrt((T(-2) * std::log(s)) / s);

	T tmp = s * sigma;
	return {
		u * tmp + mean,
		v * tmp + mean
	};
}

}  /* namespace math */
}  /* namespace dls */
