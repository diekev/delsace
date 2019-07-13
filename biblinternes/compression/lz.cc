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

using dictionnary_t = dls::tableau<dls::chaine>;

void build_dictionnary(dictionnary_t &dict)
{
	dict.reserve(256);

	for (int i = 0; i < 256; ++i) {
		dict.emplace_back(1, char(i));
	}
}

void decode(const dls::chaine &to_decode)
{
	dictionnary_t dictionnary;
	build_dictionnary(dictionnary);

	dls::chaine w(1, to_decode[0]);
	auto decoded_str = w;
	auto num_str = dls::chaine{};
	auto entree = dls::chaine{};

	auto code_point = false;

	for (auto i = 1; i < to_decode.taille(); ++i) {
		const auto &c = to_decode[i];

		if (c == '<') {
			code_point = true;
			num_str.efface();
			continue;
		}

		if (c == '>') {
			code_point = false;
		}

		if (code_point) {
			num_str += c;
			continue;
		}

		auto index = 0l;
		if (num_str.taille() != 0) {
			index = std::atoi(num_str.c_str());
			num_str.efface();
		}

		if (index > 255l && index < dictionnary.taille()) {
			entree = dictionnary[index];
		}
		else if (index > 255ul && index >= dictionnary.taille()) {
			entree = w + w[0];
		}
		else {
			entree = c;
		}

		decoded_str += entree;

		dictionnary.pousse(w + entree);
		w = entree;
	}

	std::cout << decoded_str << '\n';
}

void encode_sequence(const dls::chaine &seq,
                     const dictionnary_t &dictionnary,
                     std::ostream &os)
{
	const auto &iter = std::find(dictionnary.debut(), dictionnary.fin(), seq);
	const auto &code = iter - dictionnary.debut();

	if (code < 256) {
		os << static_cast<char>(code);
	}
	else {
		os << '<' << code << '>';
	}
}
