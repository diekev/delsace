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

#include "utf8.h"

#include <cstring>
#include <iostream>
#include <sstream>

namespace chaine {

static inline bool est_caractere_simple(const char *c_str, const size_t i)
{
	return static_cast<unsigned char>(c_str[i]) < 192;
}

static inline bool est_caractere_double(const char *c_str, const size_t i, const size_t longueur)
{
	return static_cast<unsigned char>(c_str[i]) < 224
			&& (i + 1) < longueur
			&& static_cast<unsigned char>(c_str[i + 1]) > 127;
}

static inline bool est_caractere_triple(const char *c_str, const size_t i, const size_t longueur)
{
	return static_cast<unsigned char>(c_str[i]) < 240
			&& (i + 2) < longueur
			&& static_cast<unsigned char>(c_str[i + 1]) > 127
			&& static_cast<unsigned char>(c_str[i + 2]) > 127;
}

static inline bool est_caractere_quadruple(const char *c_str, const size_t i, const size_t longueur)
{
	return static_cast<unsigned char>(c_str[i]) < 248
			&& (i + 3) < longueur
			&& static_cast<unsigned char>(c_str[i + 1]) > 127
			&& static_cast<unsigned char>(c_str[i + 2]) > 127
			&& static_cast<unsigned char>(c_str[i + 3]) > 127;
}

static inline utf8::type_valeur extrait_caractere_simple(const char *c_str, const size_t i)
{
	return static_cast<utf8::type_valeur>(c_str[i]);
}

static inline utf8::type_valeur extrait_caractere_double(const char *c_str, const size_t i)
{
	return static_cast<utf8::type_valeur>((static_cast<unsigned char>(c_str[i]) & 0x1f) << 6)
			| (static_cast<unsigned char>(c_str[i + 1]) & 0x3f);
}

static inline utf8::type_valeur extrait_caractere_triple(const char *c_str, const size_t i)
{
	return static_cast<utf8::type_valeur>((static_cast<unsigned char>(c_str[i]) & 0x0f) << 12)
			| ((static_cast<unsigned char>(c_str[i + 1]) & 0x1f) << 6)
		   | (static_cast<unsigned char>(c_str[i + 2]) & 0x3f);
}

static inline utf8::type_valeur extrait_caractere_quadruple(const char *c_str, const size_t i)
{
	return static_cast<utf8::type_valeur>((static_cast<unsigned char>(c_str[i]) & 0x07) << 18)
			| ((static_cast<unsigned char>(c_str[i + 1]) & 0x0f) << 12)
		   | ((static_cast<unsigned char>(c_str[i + 2]) & 0x1f) << 6)
		   | (static_cast<unsigned char>(c_str[i + 3]) & 0x3f);
}

static inline bool est_caractere_simple(const utf8::type_valeur i)
{
	return i < 192;
}

static inline bool est_caractere_double(const utf8::type_valeur i)
{
	return i < 2048;
}

static inline bool est_caractere_triple(const utf8::type_valeur i)
{
	return i < 63488;
}

static inline bool est_caractere_quadruple(const utf8::type_valeur i)
{
	return i < 1898496;
}

/* ************************************************************************** */

utf8::utf8(const char *c_str)
{
	auto const longueur = std::strlen(c_str);

	for (size_t i = 0; i < longueur;) {
		auto valeur = static_cast<utf8::type_valeur>(0);

		if (est_caractere_simple(c_str, i)) {
			valeur = extrait_caractere_simple(c_str, i);
			i += 1;
		}
		else if (est_caractere_double(c_str, i, longueur)) {
			valeur = extrait_caractere_double(c_str, i);
			i += 2;
		}
		else if (est_caractere_triple(c_str, i, longueur)) {
			valeur = extrait_caractere_triple(c_str, i);
			i += 3;
		}
		else if (est_caractere_quadruple(c_str, i, longueur)) {
			valeur = extrait_caractere_quadruple(c_str, i);
			i += 4;
		}
		else {
			throw "Impossible de décoder la chaine avec le codec UTF-8 !";
		}

		m_donnees.push_back(valeur);
	}
}

utf8::utf8(std::string const &std_string)
	: utf8(std_string.c_str())
{}

utf8::plage utf8::caracteres()
{
	return plage(m_donnees.begin(), m_donnees.end());
}

utf8::plage_const utf8::caracteres() const
{
	return plage_const(m_donnees.begin(), m_donnees.end());
}

utf8::type_taille utf8::taille() const
{
	return m_donnees.size();
}

utf8::type_valeur utf8::operator[](size_t i) const
{
	return m_donnees[i];
}

bool utf8::compare(utf8 const &autre) const
{
	if (this->taille() != autre.taille()) {
		return false;
	}

	for (type_taille i = 0; i < this->taille(); ++i) {
		if (m_donnees[i] != autre.m_donnees[i]) {
			return false;
		}
	}

	return true;
}

bool utf8::compare(const char *autre) const
{
	return this->compare(utf8(autre));
}

bool utf8::compare(std::string const &autre) const
{
	return this->compare(utf8(autre));
}

/* ************************************************************************** */

std::string converti_en_std_string(utf8 const &chaine_utf8)
{
	std::ostringstream os;
	os << chaine_utf8;
	return os.str();
}

/* ************************************************************************** */

std::ostream &operator<<(std::ostream &os, utf8 const &chaine_utf8)
{
	for (auto const &caractere : chaine_utf8.caracteres()) {
		if (est_caractere_simple(caractere)) {
			os << static_cast<char>(caractere);
		}
		else if (est_caractere_double(caractere)) {
			os << static_cast<char>(0xc0 | ((caractere >> 6) & 0x1f));
			os << static_cast<char>(0x80 | (caractere & 0x3f));
		}
		else if (est_caractere_triple(caractere)) {
			os << static_cast<char>(0xe0 | ((caractere >> 12) & 0x0f));
			os << static_cast<char>(0x80 | ((caractere >> 6) & 0x1f));
			os << static_cast<char>(0x80 | ((caractere & 0x3f)));
		}
		else if (est_caractere_quadruple(caractere)) {
			os << static_cast<char>(0xf0 | ((caractere >> 18) & 0x07));
			os << static_cast<char>(0x80 | ((caractere >> 12) & 0x0f));
			os << static_cast<char>(0x80 | ((caractere >> 6) & 0x1f));
			os << static_cast<char>(0x80 | (caractere & 0x3f));
		}
	}

	return os;
}

}  /* namespace chaine */
