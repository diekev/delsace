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

#include <cstring>
#include <type_traits>

/**
 * Outils pour copier la représentation binaire d'un type à un autre.
 */

template <typename T1, typename T2>
[[nodiscard]] inline auto transbit(T2 const &y)
{
	static_assert(sizeof(T1) == sizeof(T2), "les tailles des types ne sont pas égales");
	static_assert(std::is_trivially_copyable<T1>::value);
	static_assert(std::is_trivially_copyable<T2>::value);

	/* Ceci est un comportement indéfini selon le standard de C++ car seulement
	 * un élément d'une union peut être actif à la fois. Donc a est actif, mais
	 * pas b. Avec C++ 20, nous pourrons utiliser std::bit_cast. */
#if 0
	union {
		T2 a;
		T1 b;
	} conv = { y };

	return conv.b;
#else
	T1 x;
	std::memcpy(&x, &y, sizeof(T1));
	return x;
#endif
}
