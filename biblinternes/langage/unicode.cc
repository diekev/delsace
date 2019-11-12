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

#include "unicode.hh"

namespace lng {

inline bool entre(unsigned char c, unsigned char a, unsigned char b)
{
	return c >= a && c <= b;
}

int nombre_octets(const char *sequence)
{
	const auto s0 = static_cast<unsigned char>(sequence[0]);

	if (entre(s0, 0x00, 0x7F)) {
		return 1;
	}

	const auto s1 = static_cast<unsigned char>(sequence[1]);

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

		const auto s2 = static_cast<unsigned char>(sequence[2]);

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

		const auto s2 = static_cast<unsigned char>(sequence[2]);

		if (!entre(s2, 0x80, 0xBF)) {
			return 0;
		}

		const auto s3 = static_cast<unsigned char>(sequence[3]);

		if (!entre(s3, 0x80, 0xBF)) {
			return 0;
		}

		return 4;
	}

	return 0;
}

long decalage_pour_caractere(dls::vue_chaine const &chaine, long i)
{
	auto decalage = 0l;
	auto n = nombre_octets(&chaine[i]);

	while (n == 0 && i < chaine.taille()) {
		++decalage;
		++i;
		n = nombre_octets(&chaine[i]);
	}

	decalage += n;

	return decalage;
}

static std::pair<const char *, char> paires_caracteres_sans_accent[] = {
	{ "à", 'a' },
	{ "â", 'a' },
	{ "é", 'e' },
	{ "è", 'e' },
	{ "ê", 'e' },
	{ "î", 'i' },
	{ "ô", 'o' },
};

dls::chaine supprime_accents(dls::chaine const &chaine)
{
	auto res = dls::chaine();

	for (auto i = 0; i < chaine.taille();) {
		auto n = nombre_octets(&chaine[i]);

		if (n == 1) {
			res += chaine[i];
		}
		else {
			for (auto const &paire : paires_caracteres_sans_accent) {
				if (strncmp(&chaine[i], paire.first, static_cast<size_t>(n)) == 0) {
					res += paire.second;
					break;
				}
			}
		}

		i += n;
	}

	return res;
}

int converti_utf32(const char *sequence, int n)
{
	auto const s0 = static_cast<unsigned char>(sequence[0]);

	if (n == 1) {
		return static_cast<int>(s0) & 0b01111111;
	}

	auto const s1 = static_cast<unsigned char>(sequence[1]);

	if (n == 2) {
		auto valeur = (s0 & 0b00011111) << 6 | (s1 & 0b00111111);
		return valeur;
	}

	auto const s2 = static_cast<unsigned char>(sequence[2]);

	if (n == 3) {
		auto valeur = (s0 & 0b00001111) << 12 | (s1 & 0b00111111) << 6 | (s2 & 0b00111111);
		return valeur;
	}

	auto const s3 = static_cast<unsigned char>(sequence[3]);

	if (n == 4) {
		auto valeur = (s0 & 0b00000111) << 18 | (s1 & 0b00111111) << 12 | (s2 & 0b00111111) << 6 | (s3 & 0b00111111);
		return valeur;
	}

	return 0;
}

}  /* namespace lng */
