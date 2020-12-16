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

#include "biblinternes/structures/tableau.hh"

namespace dls {

template <typename T>
struct pile {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	dls::tableau<type_valeur> m_pile{};

public:
	pile() = default;

	void empile(type_reference_const valeur)
	{
		m_pile.ajoute(valeur);
	}

	type_valeur depile()
	{
		auto t = this->haut();
		m_pile.pop_back();
		return t;
	}

	type_reference haut()
	{
		return m_pile.back();
	}

	type_reference_const haut() const
	{
		return m_pile.back();
	}

	bool est_vide() const
	{
		return m_pile.est_vide();
	}

	void efface()
	{
		m_pile.efface();
	}

	type_taille taille() const
	{
		return m_pile.taille();
	}
};

template <typename T, unsigned long N>
struct pile_fixe {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	type_valeur m_pile[N];
	type_taille m_index = -1;

public:
	pile_fixe() = default;

	void empile(type_reference_const valeur)
	{
		if (m_index + 1 < N) {
			m_pile[++m_index] = valeur;
		}
	}

	type_valeur depile()
	{
		auto t = haut();
		--m_index;
		return t;
	}

	type_reference haut()
	{
		return m_pile[m_index];
	}

	type_reference_const haut() const
	{
		return m_pile[m_index];
	}

	bool est_vide() const
	{
		return m_index < 0;
	}

	bool est_pleine() const
	{
		return m_index == N;
	}

	type_taille taille() const
	{
		return m_index + 1;
	}
};

}  /* namespace dls */
