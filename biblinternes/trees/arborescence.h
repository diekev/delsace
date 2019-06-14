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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cassert>
#include <cstddef>  /* pour std::size_t */
#include <iostream>
#include <iterator>  /* pour std::bidirectional_iterator_tag et al. */

namespace foret {

/* ****************************** utilitaires ******************************* */

enum {
	parent,
	enfant
};

enum {
	forest_trailing_edge,
	forest_leading_edge,
};

template <typename I>
inline auto avance(I i) -> I
{
	return ++i;
}

template <typename I>
inline auto recule(I i) -> I
{
	return --i;
}

template <typename I>
inline auto leading_of(I i) -> I
{
	i.arete() = forest_leading_edge;
	return i;
}

template <typename I>
inline auto trailing_of(I i) -> I
{
	i.arete() = forest_trailing_edge;
	return i;
}

template <typename I>
inline auto has_children(const I &i) -> bool
{
	return !i.equal_node(avance(trailing_of(i)));
}

/* ********************************* noeud ********************************** */

/*         parent
 * ainé --  this  -- puiné
 *         enfant
 *
 * devient:
 *
 * [ parent ][ aine  ]
 * [ enfant ][ puine ]
 */
template <typename D>
class base_noeud {
public:
	enum intra_gen_t {
		aine,
		puine
	};
	enum next_prior_t {
		prior_s,
		next_s
	};

	using ptr_noeud = D*;
	using reference = ptr_noeud&;

private:
	ptr_noeud m_noeuds[2][2];

public:
	base_noeud()
	{
		this->lien(forest_leading_edge, next_s) = static_cast<ptr_noeud>(this);
		this->lien(forest_trailing_edge, prior_s) = static_cast<ptr_noeud>(this);
	}

	ptr_noeud &lien(std::size_t arete, next_prior_t lien)
	{
		return m_noeuds[arete][static_cast<std::size_t>(lien)];
	}

	ptr_noeud lien(std::size_t arete, next_prior_t lien) const
	{
		return m_noeuds[arete][static_cast<std::size_t>(lien)];
	}
};

template <typename T>
class noeud : public base_noeud<noeud<T>> {
public:
	using value_type = T;

private:
	template <typename>
	friend class iterateur_arborescence;

	value_type m_donnees;

public:
	explicit noeud(const value_type &donnees)
		: m_donnees(donnees)
	{}
};

/* *********************** reverse_fullorder_iterator *********************** */

template <typename I>
class reverse_fullorder_iterator {
	I m_base{};
	std::size_t m_arete = forest_trailing_edge;

public:
	using value_type = I&;
	using reference = I&;
	using iterator_type = I;
	using iterator_category = std::bidirectional_iterator_tag;

	reverse_fullorder_iterator() = default;

	reverse_fullorder_iterator(I x)
		: m_base(--x)
		, m_arete(forest_leading_edge - x.arete())
	{}

	template <typename U>
	reverse_fullorder_iterator(const reverse_fullorder_iterator<U> &i)
		: m_base(i.base())
		, m_arete(i.arete())
	{}

	iterator_type base()
	{
		return avance(m_base);
	}

	const std::size_t arete() const
	{
		return m_arete;
	}

	std::size_t &arete()
	{
		return m_arete;
	}

	bool equal_node(const reverse_fullorder_iterator &i)
	{
		return m_base.equal_node(i.m_base);
	}

	reverse_fullorder_iterator &operator++()
	{
		m_base.arete() = forest_leading_edge - m_arete;
		--m_base;
		m_arete = forest_leading_edge - m_base.arete();

		return *this;
	}

	reverse_fullorder_iterator &operator--()
	{
		m_base.arete() = forest_leading_edge - m_arete;
		++m_base;
		m_arete = forest_leading_edge - m_base.arete();

		return *this;
	}

	reference operator*()
	{
		return *m_base;
	}
};

template <typename T>
bool operator==(
		const reverse_fullorder_iterator<T> &lhs,
		const reverse_fullorder_iterator<T> &rhs)
{
	return (lhs.base() == rhs.base()) && (lhs.arete() == rhs.arete());
}

template <typename T>
bool operator!=(
		const reverse_fullorder_iterator<T> &lhs,
		const reverse_fullorder_iterator<T> &rhs)
{
	return !(lhs == rhs);
}

/* ************************** iterateur_arborescence ************************ */

template <typename T>
class const_iterateur_arborescence;

template <typename T>
class iterateur_arborescence {
	template <typename U>
	friend void set_next(
			iterateur_arborescence<U> lhs,
			iterateur_arborescence<U> rhs);

	template <typename>
	friend class arborescence;

	template <typename>
	friend class const_iterateur_arborescence;

	noeud<T> *m_noeud = nullptr;
	std::size_t m_arete = forest_leading_edge;

public:
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::bidirectional_iterator_tag;

	iterateur_arborescence() = default;

	iterateur_arborescence(const iterateur_arborescence &rhs)
		: m_noeud(rhs.m_noeud)
		, m_arete(rhs.m_arete)
	{}

	iterateur_arborescence(noeud<T> *n, std::size_t arete)
		: m_noeud(n)
		, m_arete(arete)
	{}

	std::size_t arete() const
	{
		return m_arete;
	}

	std::size_t &arete()
	{
		return m_arete;
	}

	bool equal_node(const iterateur_arborescence &rhs) const
	{
		return m_noeud == rhs.m_noeud;
	}

	reference operator*() const
	{
		return m_noeud->m_donnees;
	}

	iterateur_arborescence &operator++()
	{
		noeud<T> *suivant(m_noeud->lien(m_arete, noeud<T>::next_s));

		if (m_arete) {
			m_arete = static_cast<std::size_t>(suivant != m_noeud);
		}
		else {
			noeud<T> *precedent(suivant->lien(forest_leading_edge, noeud<T>::prior_s));
			m_arete = static_cast<std::size_t>(precedent == m_noeud);
		}

		m_noeud = suivant;

		if (m_noeud == suivant) {
			std::cerr << "++ Le noeud est le même que son successeur!\n";
		}

		return *this;
	}

	iterateur_arborescence &operator--()
	{
		noeud<T> *suivant(m_noeud->lien(m_arete, noeud<T>::prior_s));

		if (m_arete) {
			noeud<T> *precedent(suivant->lien(forest_trailing_edge, noeud<T>::next_s));
			m_arete = static_cast<std::size_t>(precedent != m_noeud);
		}
		else {
			m_arete = static_cast<std::size_t>(suivant == m_noeud);
		}

		m_noeud = suivant;

		if (m_noeud == suivant) {
			std::cerr << "-- Le noeud est le même que son successeur!\n";
		}

		return *this;
	}
};

template <typename T>
bool operator==(
		const iterateur_arborescence<T> &lhs,
		const iterateur_arborescence<T> &rhs)
{
	return lhs.equal_node(rhs) && lhs.arete() == rhs.arete();
}

template <typename T>
bool operator!=(
		const iterateur_arborescence<T> &lhs,
		const iterateur_arborescence<T> &rhs)
{
	return !(lhs == rhs);
}

/* *********************** const_iterateur_arborescence ********************* */

template <typename T>
class const_iterateur_arborescence {
	template <typename U>
	friend void set_next(
			const_iterateur_arborescence<U> lhs,
			const_iterateur_arborescence<U> rhs);

	template <typename>
	friend class arborescence;

	const noeud<T> *m_noeud = nullptr;
	std::size_t m_arete = forest_leading_edge;

public:
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::bidirectional_iterator_tag;

	const_iterateur_arborescence() = default;

	const_iterateur_arborescence(const iterateur_arborescence<T> &rhs)
		: m_noeud(rhs.m_noeud)
		, m_arete(rhs.m_arete)
	{}

	const_iterateur_arborescence(const const_iterateur_arborescence &rhs)
		: m_noeud(rhs.m_noeud)
		, m_arete(rhs.m_arete)
	{}

	const_iterateur_arborescence(const noeud<T> *n, std::size_t arete)
		: m_noeud(n)
		, m_arete(arete)
	{}

	std::size_t arete() const
	{
		return m_arete;
	}

	std::size_t &arete()
	{
		return m_arete;
	}

	bool equal_node(const const_iterateur_arborescence &rhs) const
	{
		return m_noeud == rhs.m_noeud;
	}

	reference operator*() const
	{
		return m_noeud->m_donnees;
	}

	const_iterateur_arborescence &operator++()
	{
		noeud<T> *suivant(m_noeud->lien(m_arete, noeud<T>::next_s));

		if (m_arete == forest_leading_edge) {
			/* Continue à descendre si nous avons des descendants. */
			m_arete = static_cast<std::size_t>(suivant != m_noeud);
		}
		else {
			noeud<T> *precedent(suivant->lien(forest_leading_edge, noeud<T>::prior_s));
			/* Si le noeud n'a pas d'ainé, descendons. */
			m_arete = static_cast<std::size_t>(precedent == m_noeud);
		}

		if (m_noeud == suivant) {
			std::cerr << "Le noeud est le même que son successeur!\n";
		}

		m_noeud = suivant;

		return *this;
	}

	const_iterateur_arborescence &operator--()
	{
		noeud<T> *suivant(m_noeud->lien(m_arete, noeud<T>::prior_s));

		if (m_arete) {
			noeud<T> *precedent(suivant->lien(forest_trailing_edge, noeud<T>::next_s));
			m_arete = static_cast<std::size_t>(precedent != m_noeud);
		}
		else {
			m_arete = static_cast<std::size_t>(suivant == m_noeud);
		}

		m_noeud = suivant;

		return *this;
	}
};

template <typename T>
bool operator==(
		const const_iterateur_arborescence<T> &lhs,
		const const_iterateur_arborescence<T> &rhs)
{
	return lhs.equal_node(rhs) && lhs.arete() == rhs.arete();
}

template <typename T>
bool operator!=(
		const const_iterateur_arborescence<T> &lhs,
		const const_iterateur_arborescence<T> &rhs)
{
	return !(lhs == rhs);
}

/* ****************************** arborescence ****************************** */

template <typename T>
class arborescence {
	using type_noeud = noeud<T>;

	base_noeud<type_noeud> m_tail{};
	std::size_t m_taille = 0;

	type_noeud *tail()
	{
		return static_cast<type_noeud *>(&m_tail);
	}

	const type_noeud *tail() const
	{
		return static_cast<const type_noeud *>(&m_tail);
	}

public:
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;

	using iterator = iterateur_arborescence<T>;
	using const_iterator = const_iterateur_arborescence<T>;
	using reverse_iterator = reverse_fullorder_iterator<iterator>;
	using const_reverse_iterator = reverse_fullorder_iterator<const_iterator>;

	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	arborescence()
	{
		set_next(this->end(), this->racine());
	}

	~arborescence()
	{
		this->clear();
	}

	size_type taille()
	{
		if (!taille_valide()) {
			iterator first(begin());
			iterator last(end());

			m_taille = static_cast<size_type>(std::distance(first, last));
		}

		return m_taille;
	}

	size_type taille() const
	{
		if (taille_valide()) {
			return m_taille;
		}

		iterator first(begin());
		iterator last(end());

		return static_cast<size_type>(std::distance(first, last));
	}

	size_type taille_max() const
	{
		return static_cast<size_type>(-1);
	}

	bool taille_valide() const
	{
		return (m_taille != 0) || (this->vide());
	}

	bool vide() const
	{
		return this->begin() == this->end();
	}

	iterator racine()
	{
		return iterator(this->tail(), forest_leading_edge);
	}

	iterator begin()
	{
		return ++racine();
	}

	iterator end()
	{
		return iterator(this->tail(), forest_trailing_edge);
	}

	const_iterator begin() const
	{
		return ++const_iterator(this->tail(), forest_leading_edge);
	}

	const_iterator end() const
	{
		return const_iterator(this->tail(), forest_trailing_edge);
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator(this->end());
	}

	reverse_iterator rend()
	{
		return reverse_iterator(this->begin());
	}

	reverse_iterator rbegin() const
	{
		return const_reverse_iterator(this->end());
	}

	reverse_iterator rend() const
	{
		return const_reverse_iterator(this->begin());
	}

	reference front()
	{
		assert(!vide());
		return *begin();
	}

	const_reference front() const
	{
		assert(!vide());
		return *begin();
	}

	reference back()
	{
		assert(!vide());
		return *(--end());
	}

	const_reference back() const
	{
		assert(!vide());
		return *(--end());
	}

	void clear()
	{
		this->erase(begin(), end());
		assert(this->vide());
	}

	iterator insert(const iterator &position, const value_type &x)
	{
		iterator resultat(new type_noeud(x), true);

		if (this->taille_valide()) {
			++m_taille;
		}

		std::cerr << "<---------------------------------------------->\n";

		set_next(recule(position), resultat);
		set_next(avance(resultat), position);

		std::cerr << "<---------------------------------------------->\n";

		return resultat;
	}

	iterator erase(const iterator &first, const iterator &last)
	{
		difference_type profondeur(0);
		iterator position(first);

		while (position != last) {
			if (position.arete() == forest_leading_edge) {
				++position;
				++profondeur;
			}
			else {
				if (profondeur > 0) {
					position = erase(position);
				}
				else {
					++position;
				}

				profondeur = std::max<difference_type>(0, profondeur - 1);
			}
		}

		return last;
	}

	iterator erase(const iterator &position)
	{
		if (taille_valide()) {
			--m_taille;
		}

		iterator leading(leading_of(position));
		iterator leading_prior(recule(leading));
		iterator leading_next(avance(leading));

		iterator trailing(trailing_of(position));
		iterator trailing_prior(recule(trailing));
		iterator trailing_next(avance(trailing));

		if (has_children(position)) {
			set_next(leading_prior, leading_next);
			set_next(trailing_prior, trailing_next);
		}
		else {
			set_next(leading_prior, trailing_next);
		}

		delete position.m_noeud;

		return position.arete() ? avance(leading_prior) : trailing_next;
	}

	void push_front(const value_type &x)
	{
		insert(this->begin(), x);
	}

	void push_back(const value_type &x)
	{
		insert(this->end(), x);
	}

	void pop_front()
	{
		erase(this->begin());
	}

	void pop_back()
	{
		erase(--this->end());
	}
};

/* ****************************** utilitaires ******************************* */

template <typename T>
void set_next(iterateur_arborescence<T> lhs, iterateur_arborescence<T> rhs)
{
	using type_noeud = noeud<T>;

	std::cerr << "Noeud : " << lhs.m_noeud
			  << ", arete : " << lhs.arete()
			  << ", frère : " << type_noeud::next_s
			  << ", " << rhs.m_noeud << '\n';

	std::cerr << "Noeud : " << rhs.m_noeud
			  << ", arete : " << rhs.arete()
			  << ", frère : " << type_noeud::prior_s
			  << ", " << lhs.m_noeud << '\n';

	lhs.m_noeud->lien(lhs.arete(), type_noeud::next_s) = rhs.m_noeud;
	rhs.m_noeud->lien(rhs.arete(), type_noeud::prior_s) = lhs.m_noeud;
}

}  /* namespace foret */
