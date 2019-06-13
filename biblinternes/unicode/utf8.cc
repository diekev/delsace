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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "utf8.hh"

namespace unicode::utf8 {

template <typename T, typename T2>
inline bool entre(T c, T2 a, T2 b)
{
	return c >= static_cast<T>(a) && c <= static_cast<T>(b);
}

int nombre_octets(const char8_t *sequence)
{
	const auto s0 = sequence[0];

	if (s0 <= 0x7F) {
		return 1;
	}

	const auto s1 = sequence[1];

	if (entre(s0, 0xC2, 0xDF)) {
		if (!entre(s1, 0x80, 0xBF)) {
			return 0;
		}

		return 2;
	}

	if (entre(s0, 0xE0, 0xEF)) {
		if (s0 == 0xE0 && !entre(s1, 0xA0, 0xBF)) {
			return 0;
		}

		if (s0 == 0xED && !entre(s1, 0x80, 0x9F)) {
			return 0;
		}

		if (!entre(s1, 0x80, 0xBF)) {
			return 0;
		}

		const auto s2 = sequence[2];

		if (!entre(s2, 0x80, 0xBF)) {
			return 0;
		}

		return 3;
	}

	if (entre(s0, 0xF0, 0xF4)) {
		if (s0 == 0xF0 && !entre(s1, 0x90, 0xBF)) {
			return 0;
		}

		if (s0 == 0xF4 && !entre(s1, 0x80, 0x8F)) {
			return 0;
		}

		if (!entre(s1, 0x80, 0xBF)) {
			return 0;
		}

		const auto s2 = sequence[2];

		if (!entre(s2, 0x80, 0xBF)) {
			return 0;
		}

		const auto s3 = sequence[3];

		if (!entre(s3, 0x80, 0xBF)) {
			return 0;
		}

		return 4;
	}

	return 0;
}

bool est_valide(const chaine_utf8 &chaine)
{
	for (size_t i = 0; i < chaine.size();) {
		auto n = nombre_octets(&chaine[0]);

		if (n == 0) {
			return false;
		}

		i += static_cast<size_t>(n);
	}

	return true;
}

}  /* namespace unicode::utf8 */
