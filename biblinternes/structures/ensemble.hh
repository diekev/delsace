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

#include <set>

namespace dls {

template <typename Cle, typename Comparaison = std::less<Cle>>
struct ensemble {
private:
	std::set<Cle, Comparaison, memoire::logeuse_guardee<Cle>> m_ensemble{};

public:
	ensemble() = default;

	using iteratrice = typename std::set<Cle, Comparaison, memoire::logeuse_guardee<Cle>>::iterator;
	using const_iteratrice = typename std::set<Cle, Comparaison, memoire::logeuse_guardee<Cle>>::const_iterator;

	iteratrice debut()
	{
		return m_ensemble.begin();
	}

	iteratrice fin()
	{
		return m_ensemble.end();
	}

	const_iteratrice debut() const
	{
		return m_ensemble.cbegin();
	}

	const_iteratrice fin() const
	{
		return m_ensemble.cend();
	}

	void efface()
	{
		m_ensemble.clear();
	}

	void efface(Cle const &valeur)
	{
		m_ensemble.erase(valeur);
	}

	void insere(Cle const &valeur)
	{
		m_ensemble.insert(valeur);
	}

	void insere(Cle &&valeur)
	{
		m_ensemble.insert(valeur);
	}

	int64_t taille() const
	{
		return static_cast<int64_t>(m_ensemble.size());
	}

	int64_t compte(Cle const &valeur) const
	{
		return static_cast<int64_t>(m_ensemble.count(valeur));
	}

	iteratrice trouve(Cle const &valeur)
	{
		return m_ensemble.find(valeur);
	}

	const_iteratrice trouve(Cle const &valeur) const
	{
		return m_ensemble.find(valeur);
	}

    bool possede(Cle const &valeur) const
    {
        return trouve(valeur) != fin();
    }

	void permute(ensemble &autre)
	{
		m_ensemble.swap(autre.m_ensemble);
	}
};

template <typename Cle, typename Comparaison = std::less<Cle>>
auto begin(ensemble<Cle, Comparaison> &ens)
{
	return ens.debut();
}

template <typename Cle, typename Comparaison = std::less<Cle>>
auto begin(ensemble<Cle, Comparaison> const &ens)
{
	return ens.debut();
}

template <typename Cle, typename Comparaison = std::less<Cle>>
auto end(ensemble<Cle, Comparaison> &ens)
{
	return ens.fin();
}

template <typename Cle, typename Comparaison = std::less<Cle>>
auto end(ensemble<Cle, Comparaison> const &ens)
{
	return ens.fin();
}

}  /* namespace dls */
