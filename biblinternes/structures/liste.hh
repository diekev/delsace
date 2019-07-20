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

#include <list>

namespace dls {

template <typename T>
struct liste {
	using type_liste = std::list<T, memoire::logeuse_guardee<T>>;

private:
	type_liste m_liste{};

public:
	void pousse(T const &v)
	{
		m_liste.push_back(v);
	}

	long taille() const
	{
		return static_cast<long>(m_liste.size());
	}

	void push_front(T const &v)
	{
		m_liste.push_front(v);
	}

	void push_front(T &&v)
	{
		m_liste.push_front(v);
	}

	void pousse(T &&v)
	{
		m_liste.push_front(v);
	}

	T &front()
	{
		return m_liste.front();
	}

	T const &front() const
	{
		return m_liste.front();
	}

	T &back()
	{
		return m_liste.back();
	}

	T const &back() const
	{
		return m_liste.back();
	}

	bool est_vide() const
	{
		return m_liste.empty();
	}

	void efface()
	{
		m_liste.clear();
	}

	using iteratrice = typename type_liste::iterator;
	using const_iteratrice = typename type_liste::const_iterator;

	iteratrice debut()
	{
		return m_liste.begin();
	}

	const_iteratrice debut() const
	{
		return m_liste.cbegin();
	}

	iteratrice fin()
	{
		return m_liste.end();
	}

	const_iteratrice fin() const
	{
		return m_liste.cend();
	}

	void insere(iteratrice ou, T const &v)
	{
		m_liste.insert(ou, v);
	}

	void insere(iteratrice ou, T &&v)
	{
		m_liste.insert(ou, v);
	}
};


template <typename T>
auto begin(liste<T> &tabl)
{
	return tabl.debut();
}

template <typename T>
auto begin(liste<T> const &tabl)
{
	return tabl.debut();
}

template <typename T>
auto end(liste<T> &tabl)
{
	return tabl.fin();
}

template <typename T>
auto end(liste<T> const &tabl)
{
	return tabl.fin();
}

}  /* namespace dls */
