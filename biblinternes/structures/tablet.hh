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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "tableau.hh"

namespace dls {

template<typename T>
struct iteratrice_crue {
protected:
	T* m_ptr{};

public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T*;
	using reference = T&;

	iteratrice_crue(T* ptr = nullptr)
		: m_ptr(ptr)
	{}

	iteratrice_crue(const iteratrice_crue<T> &autre) = default;

	~iteratrice_crue() = default;

	iteratrice_crue<T>& operator=(const iteratrice_crue<T> &autre) = default;

	iteratrice_crue<T>& operator=(T* ptr)
	{
		m_ptr = ptr;
		return *this;
	}

	operator bool() const
	{
		return m_ptr != nullptr;
	}

	bool operator==(const iteratrice_crue<T>&  autre)const
	{
		return (m_ptr ==  autre.ptr_const());
	}

	bool operator!=(const iteratrice_crue<T>&  autre)const
	{
		return (m_ptr !=  autre.ptr_const());
	}

	iteratrice_crue<T>& operator += (const difference_type&  mouvement)
	{
		m_ptr +=  mouvement;
		return *this;
	}

	iteratrice_crue<T>& operator-=(const difference_type&  mouvement)
	{
		m_ptr -=  mouvement;
		return *this;
	}

	iteratrice_crue<T>& operator++()
	{
		++m_ptr;
		return *this;
	}

	iteratrice_crue<T>& operator--()
	{
		--m_ptr;
		return *this;
	}

	iteratrice_crue<T> operator++(int)
	{
		auto temp(*this);
		++m_ptr;
		return temp;
	}

	iteratrice_crue<T> operator--(int)
	{
		auto temp(*this);
		--m_ptr;
		return temp;
	}

	iteratrice_crue<T> operator+(const difference_type&  mouvement)
	{
		auto vieux_ptr = m_ptr;
		m_ptr += mouvement;
		auto temp(*this);
		m_ptr = vieux_ptr;
		return temp;
	}

	iteratrice_crue<T> operator-(const difference_type&  mouvement)
	{
		auto vieux_ptr = m_ptr;
		m_ptr -= mouvement;
		auto temp(*this);
		m_ptr = vieux_ptr;
		return temp;
	}

	difference_type operator-(const iteratrice_crue<T>&  autre)
	{
		return std::distance(autre.ptr(),this-> ptr());
	}

	T& operator*()
	{
		return *m_ptr;
	}

	const T& operator*()const
	{
		return *m_ptr;
	}

	T* operator->()
	{
		return m_ptr;
	}

	T*  ptr()const
	{
		return m_ptr;
	}

	const T*  ptr_const()const
	{
		return m_ptr;
	}
};

template<typename T>
struct iteratrice_crue_inverse : public iteratrice_crue<T> {
	using difference_type = std::ptrdiff_t;

	iteratrice_crue_inverse(T* ptr = nullptr)
		:iteratrice_crue<T>(ptr)
	{}

	iteratrice_crue_inverse(const iteratrice_crue<T>&  autre)
	{
		this->m_ptr =  autre.ptr();
	}

	iteratrice_crue_inverse(const iteratrice_crue_inverse<T>& autre) = default;

	~iteratrice_crue_inverse() = default;

	iteratrice_crue_inverse<T>& operator=(const iteratrice_crue_inverse<T>& autre) = default;

	iteratrice_crue_inverse<T>& operator=(const iteratrice_crue<T>&  autre)
	{
		this->m_ptr =  autre.ptr();
		return *this;
	}

	iteratrice_crue_inverse<T>& operator=(T* ptr)
	{
		this->ptr(ptr);
		return *this;
	}

	iteratrice_crue_inverse<T>& operator += (const difference_type&  mouvement)
	{
		this->m_ptr -= mouvement;
		return *this;
	}

	iteratrice_crue_inverse<T>& operator-=(const difference_type&  mouvement)
	{
		this->m_ptr += mouvement;
		return *this;
	}

	iteratrice_crue_inverse<T>& operator++()
	{
		--this->m_ptr;
		return *this;
	}

	iteratrice_crue_inverse<T>& operator--()
	{
		++this->m_ptr;
		return *this;
	}

	iteratrice_crue_inverse<T> operator++(int)
	{
		auto temp(*this);
		--this->m_ptr;
		return temp;
	}

	iteratrice_crue_inverse<T> operator--(int)
	{
		auto temp(*this);
		++this->m_ptr;
		return temp;
	}

	iteratrice_crue_inverse<T> operator+(const int&  mouvement)
	{
		auto vieux_ptr = this->m_ptr;
		this->m_ptr -= mouvement;
		auto temp(*this);
		this->m_ptr = vieux_ptr;
		return temp;
	}

	iteratrice_crue_inverse<T> operator-(const int&  mouvement)
	{
		auto vieux_ptr = this->m_ptr;
		this->m_ptr += mouvement;
		auto temp(*this);
		this->m_ptr = vieux_ptr;
		return temp;
	}

	difference_type operator-(const iteratrice_crue_inverse<T>& autre)
	{
		return std::distance(this-> ptr(),autre.ptr());
	}

	iteratrice_crue<T> base()
	{
		iteratrice_crue<T> forwardIterator(this->m_ptr);
		++forwardIterator;
		return forwardIterator;
	}
};

template <class T, unsigned long TAILLE_INITIALE>
class tablet {
	T m_tablet[TAILLE_INITIALE];
	T* m_memoire = m_tablet;

	long m_alloue = static_cast<long>(TAILLE_INITIALE);
	long m_taille = 0;

public:
	tablet() = default;

	explicit tablet(long taille_initiale)
	{
		redimensionne(taille_initiale);
	}

	tablet(const tablet &autre)
	{
		copie_donnees(autre);
	}

	tablet &operator=(const tablet &autre)
	{
		copie_donnees(autre);
		return *this;
	}

	tablet(tablet &&autre)
		: tablet()
	{
		echange(autre);
	}

	tablet &operator=(tablet &&autre)
	{
		echange(autre);
		return *this;
	}

	~tablet()
	{
		supprime_donnees();
	}

	void echange(tablet &autre)
	{
		if (this->est_stocke_dans_classe() && autre.est_stocke_dans_classe()) {
			for (auto i = 0ul; i < TAILLE_INITIALE; ++i) {
				std::swap(m_tablet[i], autre.m_tablet[i]);
			}
		}
		else if (this->est_stocke_dans_classe()) {
			echange_tablet_memoire(*this, autre);
		}
		else if (autre.est_stocke_dans_classe()) {
			echange_tablet_memoire(autre, *this);
		}
		else {
			std::swap(m_memoire, autre.m_memoire);
		}

		std::swap(m_taille, autre.m_taille);
		std::swap(m_alloue, autre.m_alloue);
	}

	bool est_stocke_dans_classe() const
	{
		return m_memoire == m_tablet;
	}

	void efface()
	{
		m_taille = 0;
	}

    void pousse(T const &t)
    {
        assert(m_taille < std::numeric_limits<long>::max());
        garantie_capacite(m_taille + 1);
        m_memoire[m_taille++] = t;
    }

    void pousse(T &&t)
    {
        assert(m_taille < std::numeric_limits<long>::max());
        garantie_capacite(m_taille + 1);
        m_memoire[m_taille++] = std::move(t);
    }

	bool est_vide() const
	{
		return m_taille == 0;
	}

	T &operator[](long i)
	{
		assert(i>= 0 && i < m_taille);
		return m_memoire[i];
	}

	const T &operator[](long i) const
	{
		assert(i>= 0 && i < m_taille);
		return m_memoire[i];
	}

	long taille() const
	{
		assert(m_taille >= 0);
		return m_taille;
	}

	long capacite() const
	{
		assert(m_alloue >= static_cast<long>(TAILLE_INITIALE));
		return m_alloue;
	}

	void redimensionne(long n)
	{
		garantie_capacite(n);
		this->m_taille = n;
	}

	void reserve(long n)
	{
		garantie_capacite(n);
	}

	const T *donnees() const
	{
		assert(m_memoire);
		return m_memoire;
	}

	T *donnees()
	{
		assert(m_memoire);
		return m_memoire;
	}

	typedef iteratrice_crue<T> iterator;
	typedef iteratrice_crue<const T> const_iterator;

	typedef iteratrice_crue_inverse<T> reverse_iterator;
	typedef iteratrice_crue_inverse<const T> const_reverse_iterator;

	iterator begin() const
	{return iterator(&m_memoire[0]);}
	iterator end() const
	{return iterator(&m_memoire[m_taille]);}

	const_iterator cbegin() const {return const_iterator(&m_memoire[0]);}
	const_iterator cend() const {return const_iterator(&m_memoire[m_taille]);}

	reverse_iterator rbegin()
	{return reverse_iterator(&m_memoire[m_taille - 1]);}
	reverse_iterator rend()
	{return reverse_iterator(&m_memoire[-1]);}

	const_reverse_iterator crbegin() const {return const_reverse_iterator(&m_memoire[m_taille - 1]);}
	const_reverse_iterator crend() const {return const_reverse_iterator(&m_memoire[-1]);}


	iterator debut()
	{
		return this->begin();
	}

	const_iterator debut() const
	{
		return this->cbegin();
	}

	reverse_iterator debut_inverse()
	{
		return this->rbegin();
	}

	const_reverse_iterator debut_inverse() const
	{
		return this->crbegin();
	}

	iterator fin()
	{
		return this->end();
	}

	const_iterator fin() const
	{
		return this->cend();
	}

	reverse_iterator fin_inverse()
	{
		return this->rend();
	}

	const_reverse_iterator fin_inverse() const
	{
		return this->crend();
	}

	T defile()
	{
		auto t = m_memoire[m_taille - 1];
		m_taille -= 1;
		return t;
	}

	T &front()
	{
		return m_memoire[0];
	}

	T const &front() const
	{
		return m_memoire[0];
	}

	T &back()
	{
		return m_memoire[m_taille - 1];
	}

	T const &back() const
	{
		return m_memoire[m_taille - 1];
	}

	void pop_back()
	{
		m_taille -= 1;
	}

private:
	void garantie_capacite(long cap)
	{
		if (cap == 0) {
			return;
		}

		if (cap <= m_alloue) {
			return;
		}

		assert(cap <= std::numeric_limits<long>::max() / 2);

		auto nouvelle_capacite = cap * 2;

		if (m_memoire != m_tablet) {
			memoire::reloge_tableau("tablet", m_memoire, m_alloue, nouvelle_capacite);
		}
		else {
			m_memoire = memoire::loge_tableau<T>("tablet", nouvelle_capacite);

			for (int i = 0; i < m_taille; ++i) {
				m_memoire[i] = m_tablet[i];
			}
		}

		m_alloue = nouvelle_capacite;
	}

	void copie_donnees(tablet const &autre)
	{
		supprime_donnees();

		this->redimensionne(autre.taille());

		for (auto i = 0; i < autre.taille(); ++i) {
			m_memoire[i] = autre[i];
		}
	}

	void supprime_donnees()
    {
		if (m_memoire != m_tablet) {
			memoire::deloge_tableau("tablet", m_memoire, m_alloue);
			m_memoire = m_tablet;
			m_alloue = TAILLE_INITIALE;
		}
	}

	void echange_tablet_memoire(tablet &tablet_tablet, tablet &tablet_memoire)
	{
		for (auto i = 0ul; i < TAILLE_INITIALE; ++i) {
			std::swap(tablet_tablet.m_tablet[i], tablet_memoire.m_tablet[i]);
		}

		std::swap(tablet_tablet.m_memoire, tablet_memoire.m_memoire);
		tablet_memoire.m_memoire = tablet_memoire.m_tablet;
	}
};

} /* namespace dls */
