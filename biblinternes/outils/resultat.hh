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

#include <cassert>
#include <type_traits>

template <typename TypeTag, typename TypeDonnees>
struct tag_pour_donnees;

/* Cette classe contient une union de deux objets ayant un tag, et tient trace
 * duquel elle contient, utile pour discriminer les retours des fonctions.
 */
template <typename T1, typename T2, typename TypeTag>
struct Resultat {
private:
	union {
		T1 m_t1;
		T2 m_t2;
	};

	TypeTag tag = TypeTag::INVALIDE;

public:
	Resultat(T1 t1_)
		: m_t1(t1_)
		, tag(tag_pour_donnees<TypeTag, T1>::tag)
	{}

	Resultat(T2 t2_)
		: m_t2(t2_)
		, tag(tag_pour_donnees<TypeTag, T2>::tag)
	{}

	template <typename T>
	bool est() const
	{
		return tag == tag_pour_donnees<TypeTag, T>::tag;
	}

	template <typename T>
	T resultat()
	{
		assert(est<T>());
		if constexpr (std::is_same_v<T, T1>) {
			return m_t1;
		}
		else {
			return m_t2;
		}
	}
};
