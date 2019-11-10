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

#include "plage.hh"

namespace dls {

template <typename T, unsigned long N>
struct tableau_fixe {
	using type_valeur = T;
	using type_pointeur = type_valeur*;
	using type_reference = type_valeur&;
	using type_pointeur_const = type_valeur const*;
	using type_reference_const = type_valeur const&;
	using plage_valeur = plage_continue<type_valeur>;
	using plage_valeur_const = plage_continue<const type_valeur>;

private:
	type_valeur m_donnees[N];

	template <typename PV, typename... PVs>
	void construit_index(unsigned long &i, PV const &pv)
	{
		m_donnees[i++] = pv;
	}

public:
	tableau_fixe() = default;

	template <typename... Ts>
	constexpr tableau_fixe(type_valeur const &t, Ts &&... ts)
	{
		static_assert(sizeof...(ts) + 1 == N, "La taille de la liste n'est pas celle attendue");

		unsigned long i = 0;
		construit_index(i, t);
		(construit_index(i, ts), ...);
	}

	plage_valeur plage()
	{
		return plage_valeur(&m_donnees[0], &m_donnees[N]);
	}

	plage_valeur_const plage() const
	{
		return plage_valeur_const(&m_donnees[0], &m_donnees[N]);
	}

	plage_valeur trouve(T const &v)
	{
		auto plg = this->plage();

		while (!plg.est_finie()) {
			if (v == plg.front().premier) {
				break;
			}

			plg.effronte();
		}

		return plg;
	}
};

template <typename T, typename... Ts>
inline auto cree_tableau(T const &arg0, Ts &&... args)
{
	return tableau_fixe<T, sizeof...(args) + 1>(arg0, args...);
}

}  /* namespace dls */
