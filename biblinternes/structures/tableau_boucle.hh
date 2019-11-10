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

#include "tableau.hh"

namespace dls {

/**
 * Enveloppe autour d'un tableau pour tenir trace du dernier élément
 * accédé. Une fois le dernier élément du tableau accédé, nous
 * retournons au premier.
 */
template <typename T>
struct tableau_boucle {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	tableau<type_valeur> m_tabl{};
	long m_index = 0;

public:
	tableau_boucle() = default;

	void pousse(type_valeur &&valeur)
	{
		m_tabl.pousse(valeur);
	}

	void pousse(type_reference_const valeur)
	{
		m_tabl.pousse(valeur);
	}

	type_reference_const element()
	{
		if (m_index == m_tabl.taille()) {
			m_index = 0;
		}

		auto i = m_index;
		m_index += 1;
		return m_tabl[i];
	}
};

}  /* namespace dls */
