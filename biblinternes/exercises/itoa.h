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

template <unsigned N>
struct digit_count {
	static constexpr unsigned value = ((N < 10) ? 1 : digit_count<N / 10>::value + 1);
};

template <>
struct digit_count<0> {
	static constexpr unsigned value = 1;
};

template <unsigned N>
struct itoa {
	static constexpr char digits[] = {
	    ((N / 1000000000) % 10) + '0',
	    ((N / 100000000) % 10) + '0',
	    ((N / 10000000) % 10) + '0',
	    ((N / 1000000) % 10) + '0',
	    ((N / 100000) % 10) + '0',
	    ((N / 10000) % 10) + '0',
	    ((N / 1000) % 10) + '0',
	    ((N / 100) % 10) + '0',
	    ((N / 10) % 10) + '0',
	    ((N / 1) % 10) + '0',
	    '\0'
	};

	static constexpr unsigned size = digit_count<N>::value;

	static constexpr const char *value = digits + (sizeof(digits) - size - 1);
};

template <unsigned N>
constexpr char itoa<N>::digits[];
