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

#include "biblinternes/structures/ensemble.hh"

/* Un magasin est simplement un ensemble de valeurs connues lors de la
 * compilation, mais à qui nous ne pouvons attribuer de taille fixe pour pouvoir
 * en mettre plusieurs dans une même structure de données.
 */

namespace dls {

template <typename T>
struct magasin {
	using type_valeur = T;
	using type_pointeur = type_valeur*;
	using type_reference = type_valeur&;
	using type_pointeur_const = type_valeur const*;
	using type_reference_const = type_valeur const&;

private:
	ensemble<T> m_donnees{};

public:
	magasin() = default;

	template <typename... Ts>
	explicit constexpr magasin(type_valeur const &t, Ts &&... ts)
	{
		m_donnees.insere(t);
		(m_donnees.insere(ts), ...);
	}

	bool possede(type_valeur const &v) const
	{
		return m_donnees.trouve(v) != m_donnees.fin();
	}
};

}  /* namespace dls */
