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

#define TOUJOURS_INLINE [[ gnu::always_inline ]]
#define JAMAIS_INLINE  [[ gnu::noinline ]]

#ifndef INUTILISE
#	define INUTILISE(x) static_cast<void>(x)
#endif

#define CHAINE_IMPL(x) #x
#define CHAINE(x) CHAINE_IMPL(x)

#define PRAGMA_IMPL(x) _Pragma(#x)
#define A_FAIRE(x) PRAGMA_IMPL(message("À FAIRE : " CHAINE(x)))

#define REMBOURRE(x) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunused-private-field\"")  \
	char VARIABLE_ANONYME(_pad)[x] \
	_Pragma("clang diagnostic pop")

#define COPIE_CONSTRUCT(x) \
	x(x const &) = default; \
	x &operator=(x const &) = default
