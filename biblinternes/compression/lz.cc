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

#include "lz.h"

#include <algorithm>

using dictionnary_t = std::vector<std::string>;

void build_dictionnary(dictionnary_t &dict)
{
	dict.reserve(256);

	for (int i = 0; i < 256; ++i) {
		dict.emplace_back(1, char(i));
	}
}

void decode(const std::string &to_decode)
{
	dictionnary_t dictionnary;
	build_dictionnary(dictionnary);

	std::string w(1, to_decode[0]);
	auto decoded_str = w;
	auto num_str = std::string{};
	auto entree = std::string{};

	auto code_point = false;

	for (size_t i = 1; i < to_decode.size(); ++i) {
		const auto &c = to_decode[i];

		if (c == '<') {
			code_point = true;
			num_str.clear();
			continue;
		}

		if (c == '>') {
			code_point = false;
		}

		if (code_point) {
			num_str += c;
			continue;
		}

		auto index = 0ul;
		if (num_str.size() != 0) {
			index = static_cast<size_t>(std::atoi(num_str.c_str()));
			num_str.clear();
		}

		if (index > 255ul && index < dictionnary.size()) {
			entree = dictionnary[index];
		}
		else if (index > 255ul && index >= dictionnary.size()) {
			entree = w + w[0];
		}
		else {
			entree = c;
		}

		decoded_str += entree;

		dictionnary.push_back(w + entree);
		w = entree;
	}

	std::cout << decoded_str << '\n';
}

void encode_sequence(const std::string &seq,
                     const dictionnary_t &dictionnary,
                     std::ostream &os)
{
	const auto &iter = std::find(dictionnary.begin(), dictionnary.end(), seq);
	const auto &code = iter - dictionnary.begin();

	if (code < 256) {
		os << static_cast<char>(code);
	}
	else {
		os << '<' << code << '>';
	}
}
