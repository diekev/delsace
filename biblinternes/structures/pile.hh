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
private:
	dls::tableau<T> m_pile{};

public:
	pile() = default;

	void empile(T const &valeur)
	{
		m_pile.pousse(valeur);
	}

	void depile()
	{
		m_pile.pop_back();
	}

	T &haut()
	{
		return m_pile.back();
	}

	T const &haut() const
	{
		return m_pile.back();
	}

	bool est_vide() const
	{
		return m_pile.est_vide();
	}

	long taille() const
	{
		return m_pile.taille();
	}
};

}  /* namespace dls */
