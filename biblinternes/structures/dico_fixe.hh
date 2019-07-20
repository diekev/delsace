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

template <typename T1, typename T2>
struct paire {
	T1 premier{};
	T2 second{};

	paire() = default;

	paire(T1 const &p, T2 const &s)
		: premier(p)
		, second(s)
	{}

	paire(paire const &) = default;
	paire &operator=(paire const &) = default;
};

template <typename C, typename V, unsigned long N>
struct dico_fixe {
	using type_valeur = paire<C, V>;
	using type_pointeur = type_valeur*;
	using type_reference = type_valeur&;
	using type_pointeur_const = type_valeur const*;
	using type_reference_const = type_valeur const&;

private:
	type_valeur m_donnees[N];

	template <typename PV, typename... PVs>
	void construit_index(unsigned long &i, PV const &pv)
	{
		m_donnees[i++] = pv;
	}

public:
	template <typename PV, typename... PVs>
	constexpr dico_fixe(PV const &pv, PVs &&... pvs)
	{
		static_assert(sizeof...(pvs) + 1 == N, "La taille de la liste n'est pas celle attendue");

		unsigned long i = 0;
		construit_index(i, pv);
		(construit_index(i, pvs), ...);
	}

	using plage_valeur = plage_continue<type_valeur>;

	plage_valeur trouve(C const &v)
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

	plage_valeur trouve_binaire(C const &v)
	{
		auto plg = this->plage();

		while (!plg.est_finie()) {
			auto m = plg.deuxieme_moitie();

			/* À FAIRE : double comparaison. */
			if (m.front().premier == v) {
				plg = m;
				break;
			}

			if (m.front().premier < v) {
				m.effronte();
				plg = m;
			}
			else {
				plg = plg.premiere_moitie();
			}
		}

		return plg;
	}

	plage_valeur plage()
	{
		return plage_valeur(&m_donnees[0], &m_donnees[N]);
	}
};

template <template<typename, typename> typename... Args, typename C, typename V>
inline auto cree_dico(Args<C, V> &&... args)
{
	return dico_fixe<C, V, sizeof...(args)>(args...);
}

}  /* namespace dls */
