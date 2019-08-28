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

#include <map>

namespace dls {

template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>>
struct dico {
	using type_struct = std::map<_Key, _Tp, _Compare, memoire::logeuse_guardee<std::pair<const _Key, _Tp>>>;
	using iteratrice = typename type_struct::iterator;
	using const_iteratrice = typename type_struct::const_iterator;

	using key_type = typename type_struct::key_type;
	using size_type = typename type_struct::size_type;
	using value_type = typename type_struct::value_type;

private:
	type_struct m_dico{};

public:
	dico() = default;

	dico(std::initializer_list<value_type> init_list)
		: m_dico(init_list)
	{}

	dico &operator=(std::initializer_list<value_type> init_list)
	{
		m_dico = init_list;
		return *this;
	}

	void insere(std::pair<const _Key, _Tp> const &valeur)
	{
		m_dico.insert(valeur);
	}

	long taille() const
	{
		return static_cast<long>(m_dico.size());
	}

	long compte(_Key const &cle) const
	{
		return static_cast<long>(m_dico.count(cle));
	}

	void efface()
	{
		m_dico.clear();
	}

	void efface(const_iteratrice iter)
	{
		m_dico.erase(iter);
	}

	void efface(_Key const &cle)
	{
		m_dico.erase(cle);
	}

	bool est_vide() const
	{
		return m_dico.empty();
	}

	_Tp &operator[](_Key const &cle)
	{
		return m_dico[cle];
	}

	_Tp const &a(_Key const &cle) const
	{
		return m_dico.at(cle);
	}

	iteratrice debut()
	{
		return m_dico.begin();
	}

	iteratrice fin()
	{
		return m_dico.end();
	}

	const_iteratrice debut() const
	{
		return m_dico.cbegin();
	}

	const_iteratrice fin() const
	{
		return m_dico.cend();
	}

	iteratrice trouve(_Key const &cle)
	{
		return m_dico.find(cle);
	}

	const_iteratrice trouve(_Key const &cle) const
	{
		return m_dico.find(cle);
	}
};

template <
		typename _Key,
		typename _Tp,
		typename _Compare = std::less<_Key>
		>
auto begin(dico<_Key, _Tp, _Compare> &tabl)
{
	return tabl.debut();
}

template <
		typename _Key,
		typename _Tp,
		typename _Compare = std::less<_Key>
		>
auto begin(dico<_Key, _Tp, _Compare> const &tabl)
{
	return tabl.debut();
}

template <
		typename _Key,
		typename _Tp,
		typename _Compare = std::less<_Key>
		>
auto end(dico<_Key, _Tp, _Compare> &tabl)
{
	return tabl.fin();
}

template <
		typename _Key,
		typename _Tp,
		typename _Compare = std::less<_Key>
		>
auto end(dico<_Key, _Tp, _Compare> const &tabl)
{
	return tabl.fin();
}

}  /* namespace dls */
