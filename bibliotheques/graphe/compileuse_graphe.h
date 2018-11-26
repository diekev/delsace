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

#include <danjo/types/couleur.h>
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

	void ajoute_noeud(float x);

	void ajoute_noeud(float x, float y);

	void ajoute_noeud(float x, float y, float z);

	void ajoute_noeud(float x, float y, float z, float w);

	void ajoute_noeud(const dls::math::vec3f &v);

	size_t decalage_pile(PriseSortie *prise);

	void stocke_decimal(size_t decalage, const float &v);

	void stocke_vec3f(size_t decalage, const dls::math::vec3f &v);

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

inline couleur32 pile_charge_couleur(CompileuseGraphe::iterateur &pointeur)
{
	couleur32 c;
	c.r = *pointeur++;
	c.v = *pointeur++;
	c.b = *pointeur++;
	c.a = *pointeur++;
	return c;
}

inline void pile_stocke_entier(CompileuseGraphe::iterateur &pointeur, const int e)
{
	*pointeur++ = e;
}

inline void pile_stocke_decimal(CompileuseGraphe::iterateur &pointeur, const float d)
{
	*pointeur++ = d;
}

inline void pile_stocke_vec3f(CompileuseGraphe::iterateur &pointeur, const dls::math::vec3f &v)
{
	*pointeur++ = v.x;
	*pointeur++ = v.y;
	*pointeur++ = v.z;
}

inline void pile_stocke_couleur(CompileuseGraphe::iterateur &pointeur, const couleur32 &c)
{
	*pointeur++ = c.r;
	*pointeur++ = c.v;
	*pointeur++ = c.b;
	*pointeur++ = c.a;
}
