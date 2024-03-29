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

#include "vue_chaine.hh"
#include "vue_chaine_compacte.hh"

#include <cstring>
#include <iosfwd>
#include <string>

namespace dls {

struct chaine {
	using type_chaine = std::basic_string<char, std::char_traits<char>, memoire::logeuse_guardee<char>>;
	using iteratrice = typename type_chaine::iterator;
	using const_iteratrice = typename type_chaine::const_iterator;
	using iteratrice_inverse = typename type_chaine::reverse_iterator;
	using const_iteratrice_inverse = typename type_chaine::const_reverse_iterator;

private:
	type_chaine m_chaine{};

public:
	static constexpr int64_t npos = -1;

	chaine() = default;

	chaine(char c);

	chaine(char const *__c_str);

	chaine(char const *__c_str, int64_t taille);

	chaine(vue_chaine const &vue);

	chaine(vue_chaine_compacte const &vue);

	chaine(int64_t nombre, char c);

	template <typename AllocatorT>
	chaine(std::basic_string<char, std::char_traits<char>, AllocatorT> const &str)
		: m_chaine(str)
	{}

	template <typename AllocatorT>
	chaine(std::basic_string<char, std::char_traits<char>, AllocatorT> &&str)
		: m_chaine(str)
	{}

	template <typename __iter_horsin, typename = std::_RequireInputIter<__iter_horsin>>
	chaine(__iter_horsin __deb, __iter_horsin __fin)
		: m_chaine(__deb, __fin)
	{}

	void efface();

	void efface(iteratrice iter);

	void efface(iteratrice iter1, iteratrice iter2);

	void efface(int64_t pos, int64_t n = npos);

	void reserve(int64_t combien);

	void redimensionne(int64_t combien);

	void redimensionne(int64_t combien, char c);

	void ajoute(char c)
	{
		m_chaine.push_back(c);
	}

	chaine &append(chaine const &c);

	bool est_vide() const;

	inline int64_t taille() const
	{
		return static_cast<int64_t>(m_chaine.size());
	}

	int64_t capacite() const;

	dls::chaine sous_chaine(int64_t pos, int64_t combien = npos) const;

	int64_t trouve(char c, int64_t pos = 0) const;

	int64_t trouve(chaine const &motif, int64_t pos = 0) const;

	int64_t trouve_premier_de(char c) const;

	int64_t trouve_premier_de(chaine const &c, int64_t pos = 0) const;

	int64_t trouve_premier_non_de(char c, int64_t pos = 0) const;

	int64_t trouve_premier_non_de(chaine const &c, int64_t pos = 0) const;

	int64_t trouve_dernier_de(char c) const;

	int64_t trouve_dernier_non_de(char c) const;

	int64_t trouve_dernier_non_de(chaine const &c) const;

	void insere(int64_t pos, int64_t combien, char c);

	void remplace(int64_t pos, int64_t combien, chaine const &motif);

	inline char &operator[](int64_t idx)
	{
		return m_chaine[static_cast<size_t>(idx)];
	}

	inline char const &operator[](int64_t idx) const
	{
		return m_chaine[static_cast<size_t>(idx)];
	}

	char const *c_str() const
	{
		return m_chaine.c_str();
	}

	type_chaine const &std_str() const;

	iteratrice debut();

	iteratrice fin();

	const_iteratrice debut() const;

	const_iteratrice fin() const;

	iteratrice_inverse debut_inverse();

	iteratrice_inverse fin_inverse();

	const_iteratrice_inverse debut_inverse() const;

	const_iteratrice_inverse fin_inverse() const;

	chaine &operator+=(char c);

	chaine &operator+=(chaine const &autre);

	operator vue_chaine() const
	{
		return vue_chaine(this->c_str(), this->taille());
	}

	operator vue_chaine_compacte() const
	{
		return vue_chaine_compacte(this->c_str(), this->taille());
	}

	void permute(chaine &autre);
};

/* ************************************************************************** */

std::ostream &operator<<(std::ostream &os, chaine const &c);

std::istream &operator>>(std::istream &os, chaine &c);

bool operator==(chaine const &c1, chaine const &c2);

bool operator==(chaine const &c1, vue_chaine const &c2);

bool operator==(vue_chaine const &c1, chaine const &c2);

bool operator==(chaine const &c1, vue_chaine_compacte const &c2);

bool operator==(vue_chaine_compacte const &c1, chaine const &c2);

bool operator==(chaine const &c1, char const *c2);

bool operator==(char const *c1, chaine const &c2);

bool operator!=(chaine const &c1, chaine const &c2);

bool operator!=(chaine const &c1, vue_chaine const &c2);

bool operator!=(vue_chaine const &c1, chaine const &c2);

bool operator!=(chaine const &c1, vue_chaine_compacte const &c2);

bool operator!=(vue_chaine_compacte const &c1, chaine const &c2);

bool operator!=(chaine const &c1, char const *c2);

bool operator!=(char const *c1, chaine const &c2);

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
inline auto vers_chaine(T valeur)
{
	return chaine(std::to_string(valeur));
}

template <typename T>
inline auto vers_chaine(T *valeur)
{
	return chaine(std::to_string(reinterpret_cast<int64_t>(valeur)));
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
