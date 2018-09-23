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

#include <numero7/math/vec4.h>

enum class TypeMelange {
	NORMAL,
	ADDITION,
	SOUSTRACTION,
	MULTIPLICATION,
	DIVISION,
};

template <typename T>
numero7::math::vec4<T> melange_normal(
		const numero7::math::vec4<T> &a,
		const numero7::math::vec4<T> &b,
		const T &facteur)
{
	if (b[3] == static_cast<T>(0)) {
		return a;
	}

	return a * (static_cast<T>(1.0) - facteur) + b * facteur;
}

template <typename T>
numero7::math::vec4<T> melange(
		const numero7::math::vec4<T> &a,
		const numero7::math::vec4<T> &b,
		const T &facteur,
		const TypeMelange type_melange)
{
	switch (type_melange) {
		case TypeMelange::NORMAL:
			return melange_normal(a, b, facteur);
		case TypeMelange::ADDITION:
			return melange_normal(a, a + b, facteur);
		case TypeMelange::SOUSTRACTION:
			return melange_normal(a, a - b, facteur);
		case TypeMelange::MULTIPLICATION:
			return melange_normal(a, a * b, facteur);
		case TypeMelange::DIVISION:
			return melange_normal(a, a / b, facteur);
	}

	return a;
}
