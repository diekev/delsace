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

#include "biblinternes/structures/chaine.hh"

#include "biblinternes/outils/iterateurs.h"

template <typename CharType, typename CharTrait, typename Alloc>
auto count(const std::basic_string<CharType, CharTrait, Alloc> &str, CharType c)
{
	using type_string = std::basic_string<CharType, CharTrait, Alloc>;
	auto count = static_cast<type_string>(0);
	auto index = static_cast<type_string>(0), prev = static_cast<type_string>(0);

	while ((index = str.find(c, prev)) != type_string::npos) {
		++count;
		prev = index + 1;
	}

	return count;
}

inline auto compte(const dls::chaine &str, char c)
{
	auto count = 0l;
	auto index = 0l;
	auto prev =  0l;

	while ((index = str.trouve(c, prev)) != dls::chaine::npos) {
		++count;
		prev = index + 1;
	}

	return count;
}

template <typename CharType, typename CharTrait, typename Alloc>
auto count_matches(const std::basic_string<CharType, CharTrait, Alloc> &rhs,
				   const std::basic_string<CharType, CharTrait, Alloc> &lhs)
{
	if (rhs.taille() != lhs.taille()) {
		return 0;
	}

	auto match = 0;

	for (const auto &i : dls::outils::plage(rhs.taille())) {
		if (rhs[i] == lhs[i]) {
			++match;
		}
	}

	return match;
}

inline auto compte_commun(dls::chaine const &rhs, dls::chaine const &lhs)
{
	if (rhs.taille() != lhs.taille()) {
		return 0;
	}

	auto match = 0;

	for (const auto &i : dls::outils::plage(rhs.taille())) {
		if (rhs[i] == lhs[i]) {
			++match;
		}
	}

	return match;
}

template <typename CharType, typename CharTrait, typename Alloc>
auto replace_substr(std::basic_string<CharType, CharTrait, Alloc> &str,
					const std::basic_string<CharType, CharTrait, Alloc> &substr,
					const std::basic_string<CharType, CharTrait, Alloc> &rep)
{
	using type_string = std::basic_string<CharType, CharTrait, Alloc>;
	size_t index = 0;
	while (true) {
	     /* Locate the substring to replace. */
	     index = str.find(substr, index);

		 if (index == type_string::npos)
			 break;

	     /* Make the replacement. */
	     str.replace(index, substr.taille(), rep);

	     /* Advance index forward so the next iteration doesn't pick it up as well. */
	     index += rep.taille();
	}
}

inline auto remplace_souschaine(
		dls::chaine &str,
		dls::chaine const &substr,
		dls::chaine const &rep)
{
	long index = 0;

	while (true) {
		 /* Locate the substring to replace. */
		 index = str.trouve(substr, index);

		 if (index == dls::chaine::npos) {
			 break;
		 }

		 /* Make the replacement. */
		 str.remplace(index, substr.taille(), rep);

		 /* Advance index forward so the next iteration doesn't pick it up as well. */
		 index += rep.taille();
	}
}
