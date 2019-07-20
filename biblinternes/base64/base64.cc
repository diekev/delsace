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

#include "base64.hh"

namespace base64 {

static const dls::chaine caracteres_base64 =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

static const dls::chaine caracteres_base64_url =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789-_";

template <bool POUR_URL>
static bool est_base64(unsigned char c)
{
	if (POUR_URL) {
		return (isalnum(c) || (c == '-') || (c == '_'));
	}

	return (isalnum(c) || (c == '+') || (c == '/'));
}

template <bool POUR_URL>
static inline bool est_base64(char c)
{
	return est_base64<POUR_URL>(static_cast<unsigned char>(c));
}

/* ************************************************************************** */

static dls::chaine encode_impl(const unsigned char *chaine, unsigned long longueur, const dls::chaine &table)
{
	int i = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	dls::chaine ret;
	ret.reserve(static_cast<long>(longueur) * 3 / 4);

	while (longueur--) {
		char_array_3[i++] = *(chaine++);

		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = static_cast<unsigned char>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
			char_array_4[2] = static_cast<unsigned char>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++) {
				ret += table[char_array_4[i]];
			}

			i = 0;
		}
	}

	if (i != 0) {
		for (int j = i; j < 3; j++) {
			char_array_3[j] = '\0';
		}

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = static_cast<unsigned char>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
		char_array_4[2] = static_cast<unsigned char>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (int j = 0; (j < i + 1); j++) {
			ret += table[char_array_4[j]];
		}

		while((i++ < 3)) {
			ret += '=';
		}

	}
	return ret;

}

dls::chaine encode(const unsigned char *chaine, unsigned long longueur)
{
	return encode_impl(chaine, longueur, caracteres_base64);
}

dls::chaine encode(const char *chaine, unsigned long longueur)
{
	return encode(reinterpret_cast<const unsigned char *>(chaine), longueur);
}

dls::chaine encode(const dls::chaine &chaine)
{
	return encode(chaine.c_str(), static_cast<unsigned long>(chaine.taille()));
}

dls::chaine encode_pour_url(const unsigned char *chaine, unsigned long longueur)
{
	return encode_impl(chaine, longueur, caracteres_base64_url);
}

dls::chaine encode_pour_url(const char *chaine, unsigned long longueur)
{
	return encode_pour_url(reinterpret_cast<const unsigned char *>(chaine), longueur);
}

dls::chaine encode_pour_url(const dls::chaine &chaine)
{
	return encode_pour_url(chaine.c_str(), static_cast<unsigned long>(chaine.taille()));
}

/* ************************************************************************** */

template <bool POUR_URL>
static dls::chaine decode_impl(dls::chaine const& chaine, const dls::chaine &table)
{
	auto longueur = chaine.taille();
	int i = 0;
	auto in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];

	dls::chaine ret;
	ret.reserve(chaine.taille() * 4 / 3);

	while (longueur-- && ( chaine[in_] != '=') && est_base64<POUR_URL>(chaine[in_])) {
		char_array_4[i++] = static_cast<unsigned char>(chaine[in_]);
		in_++;

		if (i ==4) {
			for (i = 0; i <4; i++) {
				char_array_4[i] = static_cast<unsigned char>(table.trouve(static_cast<char>(char_array_4[i])));
			}

			char_array_3[0] = static_cast<unsigned char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
			char_array_3[1] = static_cast<unsigned char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
			char_array_3[2] = static_cast<unsigned char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

			for (i = 0; (i < 3); i++) {
				ret += static_cast<char>(char_array_3[i]);
			}

			i = 0;
		}
	}

	if (i) {
		for (int j = i; j <4; j++) {
			char_array_4[j] = 0;
		}

		for (int j = 0; j <4; j++) {
			char_array_4[j] = static_cast<unsigned char>(table.trouve(static_cast<char>(char_array_4[j])));
		}

		char_array_3[0] = static_cast<unsigned char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
		char_array_3[1] = static_cast<unsigned char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
		char_array_3[2] = static_cast<unsigned char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

		for (int j = 0; (j < i - 1); j++) {
			ret += static_cast<char>(char_array_3[j]);
		}
	}

	return ret;
}

dls::chaine decode(const dls::chaine &chaine)
{
	return decode_impl<false>(chaine, caracteres_base64);
}

dls::chaine decode_pour_url(const dls::chaine &chaine)
{
	return decode_impl<true>(chaine, caracteres_base64_url);
}

}
