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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "biblinternes/structures/pile.hh"

namespace pile {

/* ********************************* noeud ********************************** */

struct Noeud {
	Noeud *parent = nullptr;
	dls::tableau<Noeud *> enfants = {};

	dls::chaine nom = "";

	Noeud(const Noeud &) = default;
	Noeud &operator=(const Noeud &) = default;

	Noeud(dls::chaine nom_, bool genere_enfants)
		: nom(std::move(nom_))
	{
		if (!genere_enfants) {
			return;
		}

		enfants.ajoute(new Noeud(this->nom + "/enfant_a", false));
		enfants.ajoute(new Noeud(this->nom + "/enfant_b", false));
		enfants.ajoute(new Noeud(this->nom + "/enfant_c", false));
		enfants.ajoute(new Noeud(this->nom + "/enfant_d", false));

		for (Noeud *enfant : this->enfants) {
			enfant->parent = this;
		}
	}

	~Noeud()
	{
		for (Noeud *enfant : this->enfants) {
			delete enfant;
		}
	}
};

/* ************************* arborescence_iterator ************************** */

class arborescence_iterator {
	using iterateur_noeud = dls::tableau<Noeud *>::iteratrice;

	dls::pile<iterateur_noeud> m_pile{};

	int m_profondeur = 0;

public:
	using value_type = iterateur_noeud;
	using difference_type = std::ptrdiff_t;
	using pointer = const iterateur_noeud*;
	using reference = const iterateur_noeud&;
	using iterator_category = std::input_iterator_tag;

	arborescence_iterator() noexcept
	{
		m_profondeur = -1;
		this->m_pile.empile(iterateur_noeud());
	}

	arborescence_iterator(Noeud *noeud)
	{
		this->m_pile.empile(noeud->enfants.debut());
	}

	arborescence_iterator(const arborescence_iterator &rhs)
		: m_pile(rhs.m_pile)
		, m_profondeur(rhs.m_profondeur)
	{}

	arborescence_iterator(arborescence_iterator &&rhs)
	{
		std::swap(this->m_pile, rhs.m_pile);
		std::swap(this->m_profondeur, rhs.m_profondeur);
	}

	Noeud *operator*() const
	{
		if (m_profondeur == -1 || !*m_pile.haut()) {
			return nullptr;
		}

		return (*m_pile.haut());
	}

	Noeud **operator->() const
	{
		return &(*m_pile.haut());
	}

	int profondeur()
	{
		return this->m_profondeur;
	}

	void incremente()
	{
		++(this->m_pile.haut());

		if (this->m_pile.haut() == iterateur_noeud()) {
			if (this->m_profondeur != 0) {
				--this->m_profondeur;
				this->m_pile.depile();
				++(this->m_pile.haut());
			}
		}
		else {
			++this->m_profondeur;
			this->m_pile.empile((*m_pile.haut())->enfants.debut());
		}
	}

	arborescence_iterator &operator++()
	{
		this->incremente();
		return (*this);
	}
};

arborescence_iterator begin(arborescence_iterator iter) noexcept
{
	return iter;
}

arborescence_iterator end(const arborescence_iterator &) noexcept
{
	return {};
}

bool operator==(const arborescence_iterator &ita, const arborescence_iterator &itb) noexcept
{
	return (*ita == *itb);
}

bool operator!=(const arborescence_iterator &ita, const arborescence_iterator &itb) noexcept
{
	return !(ita == itb);
}

/* ****************************** arborescence ****************************** */

struct Arborescence {
	dls::tableau<Noeud *> noeuds = {};
	Noeud *racine = nullptr;

	~Arborescence()
	{
		delete racine;
	}

	void push_back(Noeud *noeud)
	{
		this->racine->enfants.ajoute(noeud);
	}
};

}  /* namespace pile */
