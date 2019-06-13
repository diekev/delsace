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

namespace {

bool starts_with(const std::string &str, const std::string &prefix)
{
	if (str.length() < prefix.length()) {
		return false;
	}

	return std::equal(prefix.begin(), prefix.end(), str.begin());
}

std::string trim(std::string &&str, const std::string &whitespace = " \t\n")
{
	const auto fin_string = str.find_last_not_of(whitespace);

	if (fin_string == std::string::npos) {
		return {};
	}

	str.erase(fin_string + 1);

	const auto debut_string = str.find_first_not_of(whitespace);
	str.erase(0, debut_string);

	return std::move(str);
}

std::vector<std::string> split(const std::string &str, size_t pos = 0)
{
	const char * const anySpace = " \t\r\n\v\f";

	std::vector<std::string> ret;

	while (pos != std::string::npos) {
		auto start = str.find_first_not_of(anySpace, pos);

		if (start == std::string::npos) {
			break;
		}

		auto end = str.find_first_of(anySpace, start);
		auto size = (end == std::string::npos) ? end : end-start;

		ret.emplace_back(str.substr(start, size));

		pos = end;
	}

	return ret;
}

std::tuple<std::string, std::string, std::string> partition(std::string str, const std::string &point)
{
	std::tuple<std::string, std::string, std::string> ret;

	auto i = str.find(point);

	if (i == std::string::npos) {
		// no match: string goes in 0th spot only
	}
	else {
		std::get<2>(ret) = str.substr(i + point.size());
		std::get<1>(ret) = point;
		str.resize(i);
	}

	std::get<0>(ret) = std::move(str);

	return ret;
}

template <typename I>
std::string join(I iter, I end, const std::string &delim)
{
	if (iter == end) {
		return {};
	}

	std::string ret = *iter;

	for (++iter; iter != end; ++iter) {
		ret.append(delim);
		ret.append(*iter);
	}

	return ret;
}

std::vector<std::string> regex_split(const std::string &text, const std::regex &re)
{
	auto it = std::sregex_token_iterator(text.begin(), text.end(), re, -1);
	auto end = std::sregex_token_iterator();

	std::vector<std::string> ret;

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
