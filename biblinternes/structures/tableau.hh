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

#include <vector>

namespace dls {

template <typename T>
struct tableau {
	using type_valeur = T;
	using type_pointeur = T*;
	using type_pointeur_const = T const*;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;
	using type_vecteur = std::vector<T, memoire::logeuse_guardee<T>>;

	using iteratrice = typename std::vector<T, memoire::logeuse_guardee<T>>::iterator;
	using const_iteratrice = typename std::vector<T, memoire::logeuse_guardee<T>>::const_iterator;
	using iteratrice_inverse = typename std::vector<T, memoire::logeuse_guardee<T>>::reverse_iterator;
	using const_iteratrice_inverse = typename std::vector<T, memoire::logeuse_guardee<T>>::const_reverse_iterator;

private:
	type_vecteur m_vecteur{};

public:
	tableau() = default;

	tableau(long taille_)
		: m_vecteur(static_cast<size_t>(taille_))
	{}

	tableau(long taille_, type_valeur valeur)
		: m_vecteur(static_cast<size_t>(taille_), valeur)
	{}

	tableau(tableau const &autre)
		: m_vecteur(autre.m_vecteur)
	{}

	tableau(tableau &&autre)
	{
		this->permute(autre);
	}

	template <typename __iter_horsin, typename = std::_RequireInputIter<__iter_horsin>>
	tableau(__iter_horsin __deb, __iter_horsin __fin)
		: m_vecteur(__deb, __fin)
	{}

	tableau(std::initializer_list<type_valeur> init_list)
		: m_vecteur(init_list)
	{}

	tableau &operator=(std::initializer_list<type_valeur> init_list)
	{
		m_vecteur = init_list;
		return *this;
	}

	~tableau() = default;

	tableau &operator=(tableau const &autre)
	{
		m_vecteur = autre.m_vecteur;
		return *this;
	}

	tableau &operator=(tableau &&autre)
	{
		this->permute(autre);
		return *this;
	}

	void permute(tableau &autre)
	{
		m_vecteur.swap(autre.m_vecteur);
	}

	type_reference operator[](long idx)
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_vecteur.at(static_cast<size_t>(idx));
	}

	type_reference_const operator[](long idx) const
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_vecteur.at(static_cast<size_t>(idx));
	}

	type_reference a(long idx)
	{
		return this->operator[](idx);
	}

	type_reference_const a(long idx) const
	{
		return this->operator[](idx);
	}

	bool est_vide() const
	{
		return this->taille() == 0;
	}

	void efface()
	{
		m_vecteur.clear();
	}

	void pop_front()
	{
		m_vecteur.erase(m_vecteur.begin());
	}

	void pop_back()
	{
		m_vecteur.pop_back();
	}

	void ajoute(type_reference_const valeur)
	{
		m_vecteur.push_back(valeur);
	}

	void ajoute(type_valeur &&valeur)
	{
		m_vecteur.push_back(std::move(valeur));
	}

	void reserve(long nombre)
	{
		m_vecteur.reserve(static_cast<size_t>(nombre));
	}

	void redimensionne(long nouvelle_taille)
	{
		m_vecteur.resize(static_cast<size_t>(nouvelle_taille));
	}

	void redimensionne(long nouvelle_taille, type_reference_const valeur)
	{
		m_vecteur.resize(static_cast<size_t>(nouvelle_taille), valeur);
	}

	void adapte_taille()
	{
		m_vecteur.shrink_to_fit();
	}

	long taille() const
	{
		return static_cast<long>(m_vecteur.size());
	}

	long capacite() const
	{
		return static_cast<long>(m_vecteur.capacity());
	}

	type_pointeur donnees()
	{
		return m_vecteur.data();
	}

	type_pointeur_const donnees() const
	{
		return m_vecteur.data();
	}

	type_reference front()
	{
		return m_vecteur.front();
	}

	type_reference_const front() const
	{
		return m_vecteur.front();
	}

	type_reference back()
	{
		return m_vecteur.back();
	}

	type_reference_const back() const
	{
		return m_vecteur.back();
	}

	iteratrice debut()
	{
		return m_vecteur.begin();
	}

	const_iteratrice debut() const
	{
		return m_vecteur.cbegin();
	}

	iteratrice fin()
	{
		return m_vecteur.end();
	}

	const_iteratrice fin() const
	{
		return m_vecteur.cend();
	}

	iteratrice_inverse debut_inverse()
	{
		return m_vecteur.rbegin();
	}

	const_iteratrice_inverse debut_inverse() const
	{
		return m_vecteur.crbegin();
	}

	iteratrice_inverse fin_inverse()
	{
		return m_vecteur.rend();
	}

	const_iteratrice_inverse fin_inverse() const
	{
		return m_vecteur.crend();
	}

	void erase(iteratrice iter)
	{
		m_vecteur.erase(iter);
	}

	void erase(iteratrice debut_, iteratrice fin_)
	{
		m_vecteur.erase(debut_, fin_);
	}

	void insere(iteratrice ou, type_reference_const quoi)
	{
		m_vecteur.insert(ou, quoi);
	}

	void insere(iteratrice ou, long nombre, type_reference_const quoi)
	{
		m_vecteur.insert(ou, static_cast<size_t>(nombre), quoi);
	}

	void insere(iteratrice ou, std::initializer_list<type_valeur> init)
	{
		m_vecteur.insert(ou, init);
	}

	template <typename __iter_horsin>
	void insere(iteratrice ou, __iter_horsin __deb, __iter_horsin __fin)
	{
		m_vecteur.insert(ou, __deb, __fin);
	}

	template <typename... Args>
	void emplace_back(Args &&...args)
	{
		m_vecteur.emplace_back(args...);
	}
};

template <typename T>
auto operator==(tableau<T> const &t1, tableau<T> const &t2)
{
	if (t1.taille() != t2.taille()) {
		return false;
	}

	for (auto i = 0l; i < t1.taille(); ++i) {
		if (t1[i] != t2[i]) {
			return false;
		}
	}

	return true;
}

template <typename T>
auto operator!=(tableau<T> const &t1, tableau<T> const &t2)
{
	return !(t1 == t2);
}

template <typename T>
auto begin(tableau<T> &tabl)
{
	return tabl.debut();
}

template <typename T>
auto begin(tableau<T> const &tabl)
{
	return tabl.debut();
}

template <typename T>
auto end(tableau<T> &tabl)
{
	return tabl.fin();
}

template <typename T>
auto end(tableau<T> const &tabl)
{
	return tabl.fin();
}

}  /* namespace dls */
