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

#include <type_traits>

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

#if defined(__clang__) || defined(__GNUC__)
#    define REMBOURRE(x) \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wunused-private-field\"")  \
        char VARIABLE_ANONYME(_pad)[x] \
        _Pragma("clang diagnostic pop")
#else
#    define REMBOURRE(x)
#endif

#define COPIE_CONSTRUCT(x) \
	x(x const &) = default; \
	x &operator=(x const &) = default

#define EMPECHE_COPIE(x) \
	x(x const &) = delete; \
	x &operator=(x const &) = delete

#define COPIE_CONSTRUCT_MOUV(x) \
	x(x &&) = default; \
	x &operator=(x &&) = default

#define DEFINIS_OPERATEURS_DRAPEAU_IMPL(_type_drapeau_, _type_) \
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
    } \
    inline bool drapeau_est_actif(_type_drapeau_ const v, _type_drapeau_ const d) \
    { \
        return (d & v) != _type_drapeau_(0); \
    }

#define DEFINIS_OPERATEURS_DRAPEAU(_type_drapeau_) \
    DEFINIS_OPERATEURS_DRAPEAU_IMPL(_type_drapeau_, std::underlying_type_t<_type_drapeau_>)

#define taille_de(x) static_cast<int64_t>(sizeof(x))

#define POINTEUR_NUL(Type) \
	static inline Type *nul() \
	{ \
		return static_cast<Type *>(nullptr); \
	} \
	static inline Type const *nul_const() \
	{ \
		return static_cast<Type const *>(nullptr); \
	}

#define POUR(x) for (auto &it : (x))

#define POUR_NOMME(nom, x) for (auto &nom : (x))

#define POUR_INDEX(variable)                                                                      \
    if (auto index_it = -1)                                                                       \
        for (auto &it : (variable))                                                               \
            if (++index_it, true)
