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

#include <string>

#include "../outils/iterateurs.h"

template <typename CharType>
auto count(const std::basic_string<CharType> &str, CharType c)
{
	auto count = 0ul;
	auto index = 0ul, prev = 0ul;

	while ((index = str.find(c, prev)) != std::basic_string<CharType>::npos) {
		++count;
		prev = index + 1;
	}

	return count;
}

template <typename CharType>
auto count_matches(const std::basic_string<CharType> &rhs,
                   const std::basic_string<CharType> &lhs)
{
	if (rhs.length() != lhs.length()) {
		return 0;
	}

	auto match = 0;

	for (const auto &i : dls::outils::plage(rhs.length())) {
		if (rhs[i] == lhs[i]) {
			++match;
		}
	}

	return match;
}

template <typename CharType>
auto replace_substr(std::basic_string<CharType> &str,
                    const std::basic_string<CharType> &substr,
                    const std::basic_string<CharType> &rep)
{
	size_t index = 0;
	while (true) {
	     /* Locate the substring to replace. */
	     index = str.find(substr, index);

	     if (index == std::basic_string<CharType>::npos)
			 break;

	     /* Make the replacement. */
	     str.replace(index, substr.size(), rep);

	     /* Advance index forward so the next iteration doesn't pick it up as well. */
	     index += rep.size();
	}
}
