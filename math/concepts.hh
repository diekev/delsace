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

#if defined __cpp_concepts && __cpp_concepts >= 201507

#include <type_traits>

template <typename T>
concept bool ConceptNombre = requires(T a, T b)
{
	a + b;
	a - b;
	a * b;
	a / b;
	a == b;
	a != b;
	a = b;
	a < b;
	a > b;
	a <= b;
	a >= b;
	a += b;
	a -= b;
	a *= b;
	a /= b;
};

template <typename T>
concept bool ConceptNombreEntier = requires(T a, T b)
{
	a & b;
	a >> b;
	a == b;
};

/**
 * Concept pour les valeurs décimales.
 */
template <typename T>
concept bool ConceptDecimal = std::is_floating_point<T>::value;
#else
#	define ConceptNombre typename
#	define ConceptDecimal typename
#	define ConceptNombreEntier typename
#endif
