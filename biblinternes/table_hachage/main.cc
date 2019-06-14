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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */


#include "../outils/masque_binaire.h"

#include <iostream>

#if 0
enum Controle {
	ENTREE_VIDE = -128,
	ENTREE_SUPPRIMEE = -2,
	ENTREE_SENTINELLE = -1,
};

std::size_t H1(std::size_t hash)
{
	return hash >> 7;
}

Controle H2(std::size_t hash)
{
	return hash & 0x7f;
}

template <typename Cle>
struct Groupe {
	std::uint16_t controle;
	Cle m_cles[16];

	MasqueBinaire<std::uint32_t> Match(h2_t hash) const
	{
		auto match = _mm_set1_epi8(hash);
		return MasqueBinaire<std::uint32_t>(
					_mm_movemask_epi8(_mm_cmpeq_epi8(match, controle)));
	}
};

template <typename Cle, typename Valeur, typename FonctionHachage>
class tableau {
	std::size_t m_taille;
	std::size_t m_nombre_groupes;

	std::size_t *m_controles;
	Cle *m_alveoles;

public:
	tableau() = default;
	using iterateur = int;

	iterateur cherche(const Cle &cle, std::size_t hash)
	{
		std::size_t position = H1(hash) % m_taille;

		while (true) {
			if (H2(hash) == m_controles[position] && cle = m_alveoles[position]) {
				return iterateur_a(position);
			}

			if (m_controles[position] == ENTREE_VIDE) {
				return fin();
			}

			position = (position + 1) % m_taille;
		}
	}

	/* Version avec groupe */
	iterateur cherche(const Cle &cle, std::size_t hash)
	{
		std::size_t groupe = H1(hash) % m_nombre_groupes;

		while (true) {
			Groupe g{m_controles + groupe * 16};

			for (int i : g.Match(H2(hash))) {
				if (cle == m_alveoles[groupe * 16 + i]) {
					return iterateur_a(groupe * 16 + i);
				}
			}

			if (g.MatchEmpty()) {
				return fin();
			}

			groupe = (groupe + 1) % m_nombre_groupes;
		}
	}

	void efface(iterateur it)
	{
		--m_taille;

		Groupe g{(it.controle - m_controles) / 16 * 16 + m_controles};
		*it.controle = g.MatchEmpty() ? ENTREE_VIDE : ENTREE_SUPPRIMEE;
		it.m_alveoles.~Cle;
	}

	iterateur iterateur_a(std::size_t position)
	{
		return iterateur();
	}

	iterateur debut()
	{
		return iterateur();
	}

	iterateur fin()
	{
		return iterateur();
	}
};
#endif

int main()
{
}
