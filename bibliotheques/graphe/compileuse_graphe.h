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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <vector>

#include <delsace/phys/couleur.hh>
#include <delsace/math/vecteur.hh>

struct PriseSortie;

static constexpr auto TAILLE_PILE = 256;

/* *************************************************************************** */

class CompileuseGraphe {
	std::vector<float> m_pile;
	size_t m_decalage;

public:
	using iterateur = std::vector<float>::iterator;
	using iterateur_const = std::vector<float>::const_iterator;

	CompileuseGraphe();

	template <typename T>
	void ajoute_noeud(T x)
	{
		if (m_decalage + 1 >= TAILLE_PILE) {
			throw "Il n'y a plus de place dans la pile !";
		}

		m_pile[m_decalage++] = static_cast<float>(x);
	}

	template <typename T>
	void ajoute_noeud(T x, T y)
	{
		if (m_decalage + 2 >= TAILLE_PILE) {
			throw "Il n'y a plus de place dans la pile !";
		}

		m_pile[m_decalage++] = static_cast<float>(x);
		m_pile[m_decalage++] = static_cast<float>(y);
	}

	template <typename T>
	void ajoute_noeud(T x, T y, T z)
	{
		if (m_decalage + 3 >= TAILLE_PILE) {
			throw "Il n'y a plus de place dans la pile !";
		}

		m_pile[m_decalage++] = static_cast<float>(x);
		m_pile[m_decalage++] = static_cast<float>(y);
		m_pile[m_decalage++] = static_cast<float>(z);
	}

	template <typename T>
	void ajoute_noeud(T x, T y, T z, T w)
	{
		if (m_decalage + 4 >= TAILLE_PILE) {
			throw "Il n'y a plus de place dans la pile !";
		}

		m_pile[m_decalage++] = static_cast<float>(x);
		m_pile[m_decalage++] = static_cast<float>(y);
		m_pile[m_decalage++] = static_cast<float>(z);
		m_pile[m_decalage++] = static_cast<float>(w);
	}

	void ajoute_noeud(dls::math::vec3f const &v);

	size_t decalage_pile(PriseSortie *prise);

	void stocke_decimal(size_t decalage, float const &v);

	void stocke_vec3f(size_t decalage, dls::math::vec3f const &v);

	iterateur debut();

	iterateur fin();

	iterateur_const debut() const;

	iterateur_const fin() const;
	std::vector<float> pile();
};

/* *************************************************************************** */

inline int pile_charge_entier(CompileuseGraphe::iterateur &pointeur)
{
	auto v = static_cast<int>(*pointeur);
	++pointeur;
	return v;
}

inline float pile_charge_decimal(CompileuseGraphe::iterateur &pointeur)
{
	return *pointeur++;
}

inline dls::math::vec3f pile_charge_vec3f(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::vec3f v;
	v.x = *pointeur++;
	v.y = *pointeur++;
	v.z = *pointeur++;
	return v;
}

inline dls::phys::couleur32 pile_charge_couleur(CompileuseGraphe::iterateur &pointeur)
{
	dls::phys::couleur32 c;
	c.r = *pointeur++;
	c.v = *pointeur++;
	c.b = *pointeur++;
	c.a = *pointeur++;
	return c;
}

inline void pile_stocke_entier(CompileuseGraphe::iterateur &pointeur, const int e)
{
	*pointeur++ = static_cast<float>(e);
}

inline void pile_stocke_decimal(CompileuseGraphe::iterateur &pointeur, const float d)
{
	*pointeur++ = d;
}

inline void pile_stocke_vec3f(CompileuseGraphe::iterateur &pointeur, dls::math::vec3f const &v)
{
	*pointeur++ = v.x;
	*pointeur++ = v.y;
	*pointeur++ = v.z;
}

inline void pile_stocke_couleur(CompileuseGraphe::iterateur &pointeur, dls::phys::couleur32 const &c)
{
	*pointeur++ = c.r;
	*pointeur++ = c.v;
	*pointeur++ = c.b;
	*pointeur++ = c.a;
}
