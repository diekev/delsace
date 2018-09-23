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

#pragma once

#include <string>
#include <vector>

#include "bibliotheques/outils/iterateurs.h"

namespace chaine {

/**
 * Représentation d'une chaîne de caractère encodée en UTF-8 avec une sémantique
 * de valeur.
 */
class utf8 {
	std::vector<char32_t> m_donnees{};

public:
	using type_valeur = char32_t;
	using type_taille = size_t;
	using plage = plage_iterable<std::vector<char32_t>::iterator>;
	using plage_const = plage_iterable<std::vector<char32_t>::const_iterator>;

	utf8() = default;

	utf8(const utf8 &autre) = default;
	utf8(utf8 &&autre) = default;

	utf8 &operator=(const utf8 &autre) = default;
	utf8 &operator=(utf8 &&autre) = default;

	explicit utf8(const char *c_str);

	explicit utf8(const std::string &std_string);

	plage caracteres();

	plage_const caracteres() const;

	type_taille taille() const;

	type_valeur operator[](size_t i) const;

	bool compare(const utf8 &autre) const;

	bool compare(const char *autre) const;

	bool compare(const std::string &autre) const;
};

/* ********************************* égalité ******************************** */

inline bool operator==(const utf8 &a, const utf8 &b)
{
	return a.compare(b);
}

inline bool operator==(const utf8 &a, const char *b)
{
	return a.compare(b);
}

inline bool operator==(const char *a, const utf8 &b)
{
	return b.compare(a);
}

inline bool operator==(const utf8 &a, const std::string &b)
{
	return a.compare(b);
}

inline bool operator==(const std::string &a, const utf8 &b)
{
	return b.compare(a);
}

/* ******************************** inégalité ******************************* */

inline bool operator!=(const utf8 &a, const utf8 &b)
{
	return !(a == b);
}

inline bool operator!=(const utf8 &a, const char *b)
{
	return !(a == b);
}

inline bool operator!=(const char *a, const utf8 &b)
{
	return !(a == b);
}

inline bool operator!=(const utf8 &a, const std::string &b)
{
	return !(a == b);
}

inline bool operator!=(const std::string &a, const utf8 &b)
{
	return !(a == b);
}

/* ******************************* conversion ******************************* */

std::string converti_en_std_string(const utf8 &chaine_utf8);

/* ******************************* impression ******************************* */

std::ostream &operator<<(std::ostream &os, const utf8 &chaine_utf8);

}  /* namespace chaine */
