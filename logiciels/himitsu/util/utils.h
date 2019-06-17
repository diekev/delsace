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

#include <cassert>

class QSpinBox;

void rotate_wheel(QSpinBox *spin_box, int increment);

inline int letter_index(const char letter)
{
	assert(isalpha(letter));
	return letter - ((isupper(letter)) ? 'A' : 'a');
}

inline char letter_add(const char letter, const int num)
{
	char first = ((isupper(letter)) ? 'A' : 'a');
	return (((letter - first) + num) % 26) + first;
}

inline char letter_sub(const char letter, const int num)
{
	char first = ((isupper(letter)) ? 'A' : 'a');
	return (((letter - first) + 26 - num) % 26) + first;
}

/**
 * Encode and decode a char using the Beaudot table.
 */
int beaudot_encode(const char ch);
char beaudot_decode(const int code);

/**
 * Return true if both arguments are equal.
 */
template<typename T1, typename T2>
auto is_elem(T1 &&a, T2 &&b) -> bool
{
	return a == b;
}

/**
 * Return true if the first argument is equal to one of the other arguments.
 */
template<typename T1, typename T2, typename... Ts>
auto is_elem(T1 &&a, T2 &&b, Ts &&... t) -> bool
{
	return a == b || is_elem(a, t...);
}
