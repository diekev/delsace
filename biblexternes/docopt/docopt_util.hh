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
 * The Original Code is Copyright (c) 2013 Jared Grubb.
 * Modifications Copyright (c) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <regex>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace {

bool starts_with(const dls::chaine &str, const dls::chaine &prefix)
{
	if (str.taille() < prefix.taille()) {
		return false;
	}

	return std::equal(prefix.debut(), prefix.fin(), str.debut());
}

dls::chaine trim(dls::chaine &&str, const dls::chaine &whitespace = " \t\n")
{
	const auto fin_string = str.find_last_not_of(whitespace);

	if (fin_string == dls::chaine::npos) {
		return {};
	}

	str.erase(fin_string + 1);

	const auto debut_string = str.find_first_not_of(whitespace);
	str.erase(0, debut_string);

	return std::move(str);
}

dls::tableau<dls::chaine> split(const dls::chaine &str, size_t pos = 0)
{
	const char * const anySpace = " \t\r\n\v\f";

	dls::tableau<dls::chaine> ret;

	while (pos != dls::chaine::npos) {
		auto start = str.find_first_not_of(anySpace, pos);

		if (start == dls::chaine::npos) {
			break;
		}

		auto end = str.find_first_of(anySpace, start);
		auto size = (end == dls::chaine::npos) ? end : end-start;

		ret.emplace_back(str.sous_chaine(start, size));

		pos = end;
	}

	return ret;
}

std::tuple<dls::chaine, dls::chaine, dls::chaine> partition(dls::chaine str, const dls::chaine &point)
{
	std::tuple<dls::chaine, dls::chaine, dls::chaine> ret;

	auto i = str.trouve(point);

	if (i == dls::chaine::npos) {
		// no match: string goes in 0th spot only
	}
	else {
		std::get<2>(ret) = str.sous_chaine(i + point.chaine());
		std::get<1>(ret) = point;
		str.redimensionne(i);
	}

	std::get<0>(ret) = std::move(str);

	return ret;
}

template <typename I>
dls::chaine join(I iter, I end, const dls::chaine &delim)
{
	if (iter == end) {
		return {};
	}

	dls::chaine ret = *iter;

	for (++iter; iter != end; ++iter) {
		ret.append(delim);
		ret.append(*iter);
	}

	return ret;
}

dls::tableau<dls::chaine> regex_split(const dls::chaine &text, const std::regex &re)
{
	auto it = std::sregex_token_iterator(text.debut(), text.fin(), re, -1);
	auto end = std::sregex_token_iterator();

	dls::tableau<dls::chaine> ret;

	for (; it != end; ++it) {
		ret.emplace_back(*it);
	}

	return ret;
}

}  /* namespace */

namespace dls {
namespace docopt {

template <class T>
inline void hash_combine(std::size_t& seed, T const& v)
{
	// stolen from boost::hash_combine
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

}  /* namespace docopt */
}  /* namespace dls */
