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

#include "biblinternes/structures/file.hh"

namespace dls {

template <typename T>
struct ramasse_miette {
private:
	dls::file<T> m_miettes{};
	T m_valeur_nulle{};

public:
	explicit ramasse_miette(T &&valeur_nulle)
		: m_valeur_nulle(valeur_nulle)
	{}

	void valeur_nulle(T const &valeur)
	{
		m_valeur_nulle = valeur;
	}

	void ajoute_miette(T const &valeur)
	{
		m_miettes.enfile(valeur);
	}

	T trouve_miette()
	{
		if (m_miettes.est_vide()) {
			return m_valeur_nulle;
		}

		auto idx = m_miettes.front();
		m_miettes.effronte();
		return idx;
	}

	long nombre_miettes() const
	{
		return m_miettes.taille();
	}
};

}  /* namespace dls */
