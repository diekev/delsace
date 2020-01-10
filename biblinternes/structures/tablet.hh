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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "tableau.hh"

namespace dls {

template <typename T, long N>
struct tablet {
	using iterateur = T*;
	using iterateur_const = T const *;

private:
	T m_donnees[N];
	long m_taille = 0;

	tableau<T> m_tabl{};

public:
	tablet() = default;

	void pousse(T const &valeur)
	{
		if (taille() < N) {
			m_donnees[taille()] = valeur;
		}
		else if (taille() == N) {
			m_tabl.reserve(N + 1);

			for (auto const &v : m_donnees) {
				m_tabl.pousse(v);
			}

			m_tabl.pousse(valeur);
		}
		else {
			m_tabl.pousse(valeur);
		}

		m_taille += 1;
	}

	void pousse(T &&valeur)
	{
		if (taille() < N) {
			m_donnees[taille()] = valeur;
		}
		else if (taille() == N) {
			m_tabl.reserve(N + 1);

			for (auto const &v : m_donnees) {
				m_tabl.pousse(v);
			}

			m_tabl.pousse(valeur);
		}
		else {
			m_tabl.pousse(valeur);
		}

		m_taille += 1;
	}

	long taille() const
	{
		return m_taille;
	}

	iterateur begin()
	{
		if (taille() <= N) {
			return &m_donnees[0];
		}

		return &m_tabl[0];
	}

	iterateur_const begin() const
	{
		if (taille() <= N) {
			return &m_donnees[0];
		}

		return &m_tabl[0];
	}

	iterateur fin()
	{
		return begin() + taille();
	}

	iterateur_const fin() const
	{
		return begin() + taille();
	}
};

}  /* namespace dls */
