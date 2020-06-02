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

#define CONCATENE_IMPL(s1, s2) s1##s2
#define CONCATENE(s1, s2) CONCATENE_IMPL(s1, s2)

#ifndef __COUNTER__
#	define VARIABLE_ANONYME(str) CONCATENE(str, __COUNTER__)
#else
#	define VARIABLE_ANONYME(str) CONCATENE(str, __LINE__)
#endif

#define ENLIGNE_TOUJOURS [[ gnu::always_inline ]]
#define ENLIGNE_JAMAIS   [[ gnu::noinline ]]

#define INUTILISE(x) static_cast<void>(x)

#define CHAINE_IMPL(x) #x
#define CHAINE(x) CHAINE_IMPL(x)

#define PRAGMA_IMPL(x) _Pragma(#x)
#define A_FAIRE(x) PRAGMA_IMPL(message("À FAIRE : " CHAINE(x)))

#define PROBABLE(x) (__builtin_expect((x), 1))
#define IMPROBABLE(x) (__builtin_expect((x), 0))

#define REMBOURRE(x) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunused-private-field\"")  \
	char VARIABLE_ANONYME(_pad)[x] \
	_Pragma("clang diagnostic pop")

#define COPIE_CONSTRUCT(x) \
	x(x const &) = default; \
	x &operator=(x const &) = default

#define DEFINIE_OPERATEURS_DRAPEAU(_type_drapeau_, _type_) \
	inline constexpr auto operator&(_type_drapeau_ lhs, _type_drapeau_ rhs) \
	{ \
		return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) & static_cast<_type_>(rhs)); \
	} \
	inline constexpr auto operator&(_type_drapeau_ lhs, _type_ rhs) \
	{ \
		return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) & rhs); \
	} \
	inline constexpr auto operator|(_type_drapeau_ lhs, _type_drapeau_ rhs) \
	{ \
		return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) | static_cast<_type_>(rhs)); \
	} \
	inline constexpr auto operator^(_type_drapeau_ lhs, _type_drapeau_ rhs) \
	{ \
		return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) ^ static_cast<_type_>(rhs)); \
	} \
	inline constexpr auto operator~(_type_drapeau_ lhs) \
	{ \
		return static_cast<_type_drapeau_>(~static_cast<_type_>(lhs)); \
	} \
	inline constexpr auto &operator&=(_type_drapeau_ &lhs, _type_drapeau_ rhs) \
	{ \
		return (lhs = lhs & rhs); \
	} \
	inline constexpr auto &operator|=(_type_drapeau_ &lhs, _type_drapeau_ rhs) \
	{ \
		return (lhs = lhs | rhs); \
	} \
	inline constexpr auto &operator^=(_type_drapeau_ &lhs, _type_drapeau_ rhs) \
	{ \
		return (lhs = lhs ^ rhs); \
	}
