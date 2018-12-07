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

template <typename T>
void converti_hsv_rvb(T &r, T &v, T &b, const T h, const T s, const T fV)
{
	const auto chroma = fV * s;
	const auto fHPrime = std::fmod(h / static_cast<T>(60.0), 6);
	const auto fX = chroma * static_cast<T>(1 - std::fabs(std::fmod(fHPrime, 2) - 1));
	const auto fM = fV - chroma;

	if (0 <= fHPrime && fHPrime < 1) {
		r = chroma;
		v = fX;
		b = 0;
	}
	else if (1 <= fHPrime && fHPrime < 2) {
		r = fX;
		v = chroma;
		b = 0;
	}
	else if (2 <= fHPrime && fHPrime < 3) {
		r = 0;
		v = chroma;
		b = fX;
	}
	else if (3 <= fHPrime && fHPrime < 4) {
		r = 0;
		v = fX;
		b = chroma;
	}
	else if (4 <= fHPrime && fHPrime < 5) {
		r = fX;
		v = 0;
		b = chroma;
	}
	else if (5 <= fHPrime && fHPrime < 6) {
		r = chroma;
		v = 0;
		b = fX;
	}
	else {
		r = 0;
		v = 0;
		b = 0;
	}

	r += fM;
	v += fM;
	b += fM;
}
