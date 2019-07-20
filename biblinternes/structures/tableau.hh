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

//#include <cstring>

#if 1
#include <vector>

namespace dls {

/* pour l'instant enrobe un dls::tableau, notre tableau a des bogues et crash
 * trop lors des réallocations, en plus d'être lent lors des réallocations */
template <typename T>
struct tableau {
	using type_vecteur = std::vector<T, memoire::logeuse_guardee<T>>;
	using value_type = typename type_vecteur::value_type;
	using reference = T&;
	using const_reference = const T&;
	using size_type = long;

private:
	type_vecteur m_vecteur{};

public:
	tableau() = default;

	tableau(long taille_)
		: m_vecteur(static_cast<size_t>(taille_))
	{}

	tableau(long taille_, T valeur)
		: m_vecteur(static_cast<size_t>(taille_), valeur)
	{}

	tableau(tableau const &autre)
		: m_vecteur(autre.m_vecteur)
	{}

	tableau(tableau &&autre)
	{
		this->echange(autre);
	}

	template <typename __iter_horsin, typename = std::_RequireInputIter<__iter_horsin>>
	tableau(__iter_horsin __deb, __iter_horsin __fin)
		: m_vecteur(__deb, __fin)
	{}

	tableau(std::initializer_list<T> init_list)
		: m_vecteur(init_list)
	{}

	tableau &operator=(std::initializer_list<T> init_list)
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
		this->echange(autre);
		return *this;
	}

	void echange(tableau &autre)
	{
		m_vecteur.swap(autre.m_vecteur);
	}

	T &operator[](long idx)
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_vecteur.at(static_cast<size_t>(idx));
	}

	T const &operator[](long idx) const
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_vecteur.at(static_cast<size_t>(idx));
	}

	T &a(long idx)
	{
		return this->operator[](idx);
	}

	T const &a(long idx) const
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

	void pousse(T const &valeur)
	{
		m_vecteur.push_back(valeur);
	}

	void pousse(T &&valeur)
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

	void redimensionne(long nouvelle_taille, T const &valeur)
	{
		m_vecteur.resize(static_cast<size_t>(nouvelle_taille), valeur);
	}

	long taille() const
	{
		return static_cast<long>(m_vecteur.size());
	}

	long capacite() const
	{
		return static_cast<long>(m_vecteur.capacity());
	}

	T *donnees()
	{
		return m_vecteur.data();
	}

	T const *donnees() const
	{
		return m_vecteur.data();
	}

	T &front()
	{
		return m_vecteur.front();
	}

	T const &front() const
	{
		return m_vecteur.front();
	}

	T &back()
	{
		return m_vecteur.back();
	}

	T const &back() const
	{
		return m_vecteur.back();
	}

	using iteratrice = typename std::vector<T, memoire::logeuse_guardee<T>>::iterator;
	using const_iteratrice = typename std::vector<T, memoire::logeuse_guardee<T>>::const_iterator;
	using iteratrice_inverse = typename std::vector<T, memoire::logeuse_guardee<T>>::reverse_iterator;
	using const_iteratrice_inverse = typename std::vector<T, memoire::logeuse_guardee<T>>::const_reverse_iterator;

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

	void insere(iteratrice ou, T const &quoi)
	{
		m_vecteur.insert(ou, quoi);
	}

	void insere(iteratrice ou, long nombre, T const &quoi)
	{
		m_vecteur.insert(ou, static_cast<size_t>(nombre), quoi);
	}

	void insere(iteratrice ou, std::initializer_list<T> init)
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
#else
template <typename T>
struct tableau {
private:
	T *m_donnees = nullptr;
	long m_taille = 0;
	long m_capacite = 0;

	void loge_donnees(long nouvelle_taille)
	{
		m_donnees = memoire::loge_tableau<T>(nouvelle_taille);
		m_capacite = nouvelle_taille;
	}

	void reloge_donnees(long nouvelle_taille)
	{
		if (m_donnees == nullptr) {
			loge_donnees(nouvelle_taille);
			return;
		}

		memoire::reloge_tableau(m_donnees, m_capacite, nouvelle_taille);
		m_capacite = nouvelle_taille;
	}

	void deloge_donnees()
	{
		memoire::deloge_tableau(m_donnees, m_capacite);
	}

public:
	tableau() = default;

	tableau(long taille_)
		: m_taille(taille_)
	{
		loge_donnees(taille_);
	}

	tableau(long taille_, T valeur)
		: tableau(taille_)
	{
		for (auto i = 0; i < this->taille(); ++i) {
			m_donnees[i] = valeur;
		}
	}

	tableau(tableau const &autre)
	{
		reloge_donnees(autre.taille());
		std::memcpy(m_donnees, autre.m_donnees, static_cast<size_t>(this->taille()) * sizeof(T));
	}

	tableau(tableau &&autre)
	{
		this->echange(autre);
	}

	~tableau()
	{
		deloge_donnees();
	}

	tableau &operator=(tableau const &autre)
	{
		reloge_donnees(autre.taille());
		std::memcpy(m_donnees, autre.m_donnees, static_cast<size_t>(this->taille()) * sizeof(T));
		return *this;
	}

	tableau &operator=(tableau &&autre)
	{
		this->echange(autre);
		return *this;
	}

	void echange(tableau &autre)
	{
		std::swap(autre.m_taille, m_taille);
		std::swap(autre.m_capacite, m_capacite);
		std::swap(autre.m_donnees, m_donnees);
	}

	T &operator[](long idx)
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_donnees[idx];
	}

	T const &operator[](long idx) const
	{
		assert(idx >= 0);
		assert(idx < taille());
		return m_donnees[idx];
	}

	T &a(long idx)
	{
		return this->operator[](idx);
	}

	T const &a(long idx) const
	{
		return this->operator[](idx);
	}

	bool est_vide() const
	{
		return m_taille == 0;
	}

	void clear()
	{
		m_taille = 0;
	}

	void pousse(T const &valeur)
	{
		if (m_taille == m_capacite) {
			reloge_donnees(m_taille + 1);
		}

		m_donnees[m_taille] = valeur;
		++m_taille;
	}

	void reserve(long nombre)
	{
		if (this->taille() >= nombre) {
			return;
		}

		if (this->capacite() >= nombre) {
			return;
		}

		reloge_donnees(nombre);
	}

	void redimensionne(long nouvelle_taille)
	{
		if (this->capacite() >= nouvelle_taille) {
			return;
		}

		reloge_donnees(nouvelle_taille);
		m_taille = nouvelle_taille;
	}

	void redimensionne(long nouvelle_taille, T const &valeur)
	{
		this->redimensionne(nouvelle_taille);

		for (auto i = 0; i < this->taille(); ++i) {
			m_donnees[i] = valeur;
		}
	}

	long taille() const
	{
		return m_taille;
	}

	long capacite() const
	{
		return m_capacite;
	}

	T *donnees() const
	{
		return m_donnees;
	}

	T &front()
	{
		return m_donnees[0];
	}

	T const &front() const
	{
		return m_donnees[0];
	}

	T &back()
	{
		return m_donnees[m_taille - 1];
	}

	T const &back() const
	{
		return m_donnees[m_taille - 1];
	}

	struct iteratrice {
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = long;
		using pointer = T*;
		using reference = T&;

		T *ptr;

		iteratrice(T *p)
			: ptr(p)
		{}

		T &operator*()
		{
			return *ptr;
		}

		T const &operator*() const
		{
			return *ptr;
		}

		iteratrice &operator++()
		{
			++ptr;
			return *this;
		}

		iteratrice &operator--()
		{
			--ptr;
			return *this;
		}

		bool est_egal(iteratrice autre) const
		{
			return ptr == autre.ptr;
		}
	};

	iteratrice debut()
	{
		return iteratrice(m_donnees);
	}

	iteratrice debut() const
	{
		return iteratrice(m_donnees);
	}

	iteratrice fin()
	{
		return iteratrice(m_donnees + m_taille);
	}

	iteratrice fin() const
	{
		return iteratrice(m_donnees + m_taille);
	}

	/* fonction amie car le paramètre de template rend la résolution de
	 * l'opérateur parmi l'ensemble des opérateurs disponibles difficile lors de
	 * la compilation */
	friend bool operator==(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return ita.est_egal(itb);
	}

	/* fonction amie car le paramètre de template rend la résolution de
	 * l'opérateur parmi l'ensemble des opérateurs disponibles difficile lors de
	 * la compilation */
	friend bool operator!=(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return !(ita == itb);
	}

	friend long operator-(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return static_cast<long>(ita.ptr - itb.ptr);
	}

	friend auto operator+(typename tableau::iteratrice ita, long decalage)
	{
		return typename tableau::iteratrice(ita.ptr + decalage);
	}

	friend auto operator-(typename tableau::iteratrice ita, long decalage)
	{
		return typename tableau::iteratrice(ita.ptr - decalage);
	}

	friend bool operator<(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return ita.ptr < itb.ptr;
	}

	friend bool operator<=(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return ita.ptr <= itb.ptr;
	}

	friend bool operator>(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return ita.ptr > itb.ptr;
	}

	friend bool operator>=(typename tableau::iteratrice ita, typename tableau::iteratrice itb)
	{
		return ita.ptr >= itb.ptr;
	}

	void erase(iteratrice iter)
	{
		std::rotate(iter, iter + 1, this->fin());
		m_taille -= 1;
	}

	void insere(iteratrice ou, T const &quoi)
	{
		// À FAIRE
	}
};
#endif

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
