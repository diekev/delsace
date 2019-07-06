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

#include "biblinternes/outils/iterateurs.h"

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
	using plage = dls::outils::plage_iterable<std::vector<char32_t>::iterator>;
	using plage_const = dls::outils::plage_iterable<std::vector<char32_t>::const_iterator>;

	utf8() = default;

	utf8(utf8 const &autre) = default;
	utf8(utf8 &&autre) = default;

	utf8 &operator=(utf8 const &autre) = default;
	utf8 &operator=(utf8 &&autre) = default;

	explicit utf8(const char *c_str);

	explicit utf8(std::string const &std_string);

	plage caracteres();

	plage_const caracteres() const;

	type_taille taille() const;

	type_valeur operator[](size_t i) const;

	bool compare(utf8 const &autre) const;

	bool compare(const char *autre) const;

	bool compare(std::string const &autre) const;
};

/* ********************************* égalité ******************************** */

inline bool operator==(utf8 const &a, utf8 const &b)
{
	return a.compare(b);
}

inline bool operator==(utf8 const &a, const char *b)
{
	return a.compare(b);
}

inline bool operator==(const char *a, utf8 const &b)
{
	return b.compare(a);
}

inline bool operator==(utf8 const &a, std::string const &b)
{
	return a.compare(b);
}

inline bool operator==(std::string const &a, utf8 const &b)
{
	return b.compare(a);
}

/* ******************************** inégalité ******************************* */

inline bool operator!=(utf8 const &a, utf8 const &b)
{
	return !(a == b);
}

inline bool operator!=(utf8 const &a, const char *b)
{
	return !(a == b);
}

inline bool operator!=(const char *a, utf8 const &b)
{
	return !(a == b);
}

inline bool operator!=(utf8 const &a, std::string const &b)
{
	return !(a == b);
}

inline bool operator!=(std::string const &a, utf8 const &b)
{
	return !(a == b);
}

/* ******************************* conversion ******************************* */

std::string converti_en_std_string(utf8 const &chaine_utf8);

/* ******************************* impression ******************************* */

std::ostream &operator<<(std::ostream &os, utf8 const &chaine_utf8);

}  /* namespace chaine */
