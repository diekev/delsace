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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/memoire/logeuse_gardee.hh"

#include "biblinternes/structures/vue_chaine.hh"

#include <cstring>
#include <string>
#include <iostream>

namespace dls {

struct chaine {
	using type_chaine = std::basic_string<char, std::char_traits<char>, memoire::logeuse_guardee<char>>;
	using iteratrice = typename type_chaine::iterator;
	using const_iteratrice = typename type_chaine::const_iterator;

private:
	type_chaine m_chaine{};

public:
	static constexpr long npos = -1;

	chaine() = default;

	chaine(char const *__c_str);

	chaine(vue_chaine const &vue);

	template <typename AllocatorT>
	chaine(std::basic_string<char, std::char_traits<char>, AllocatorT> const &str)
		: m_chaine(str)
	{}

	template <typename AllocatorT>
	chaine(std::basic_string<char, std::char_traits<char>, AllocatorT> &&str)
		: m_chaine(str)
	{}

	void efface();

	void reserve(long combien);

	void redimensionne(long combien);

	void redimensionne(long combien, char c);

	void pousse(char c);

	chaine &append(chaine const &c);

	bool est_vide() const;

	long taille() const;

	long capacite() const;

	dls::chaine sous_chaine(long pos, long combien) const;

	long trouve(chaine const &motif) const;

	long trouve_premier_de(char c) const;

	long trouve_dernier_de(char c) const;

	void insere(long pos, long combien, char c);

	void remplace(long pos, long combien, chaine const &motif);

	char &operator[](long idx);

	char const &operator[](long idx) const;

	char const *c_str() const;

	type_chaine const &std_str() const;

	iteratrice debut();

	iteratrice fin();

	const_iteratrice debut() const;

	const_iteratrice fin() const;

	chaine &operator+=(char c);

	chaine &operator+=(chaine const &autre);

	operator vue_chaine() const;
};

/* ************************************************************************** */

std::ostream &operator<<(std::ostream &os, chaine const &c);

bool operator==(chaine const &c1, chaine const &c2);

bool operator!=(chaine const &c1, chaine const &c2);

bool operator<(chaine const &c1, chaine const &c2);

chaine operator+(chaine const &c1, chaine const &c2);

chaine operator+(char const *c1, chaine const &c2);

chaine operator+(chaine const &c1, char const *c2);

/* ************************************************************************** */

inline auto begin(chaine &c)
{
	return c.debut();
}

inline auto begin(chaine const &c)
{
	return c.debut();
}

inline auto end(chaine &c)
{
	return c.fin();
}

inline auto end(chaine const &c)
{
	return c.fin();
}

/* ************************************************************************** */

template <typename T>
auto vers_chaine(T const &valeur)
{
	return chaine(std::to_string(valeur));
}

}  /* namespace dls */

/* ************************************************************************** */

namespace std {

template <>
struct hash<dls::chaine> {
	std::size_t operator()(dls::chaine const &chn) const
	{
		auto h = std::hash<std::string>{};
		return h(chn.c_str());
	}
};

}  /* namespace std */
