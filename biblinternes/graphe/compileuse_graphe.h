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

#include "biblinternes/structures/tableau.hh"

#include "biblinternes/phys/couleur.hh"
#include "biblinternes/math/matrice.hh"
#include "biblinternes/math/vecteur.hh"

#include "noeud.h"

struct PriseSortie;

static constexpr auto TAILLE_PILE = 256;

/* *************************************************************************** */

class CompileuseGraphe {
	dls::tableau<float> m_pile;
	long m_decalage;

public:
	using iterateur = dls::tableau<float>::iteratrice;
	using iterateur_const = dls::tableau<float>::const_iteratrice;

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

	long decalage_pile(PriseSortie *prise);

	long decalage_pile(type_prise tprise);

	int decalage_pile(int taille);

	void stocke_decimal(long decalage, float const &v);

	void stocke_vec3f(long decalage, dls::math::vec3f const &v);

	iterateur debut();

	iterateur fin();

	iterateur_const debut() const;

	iterateur_const fin() const;
	dls::tableau<float> pile();
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

inline auto pile_charge_vec2(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::vec2f v;
	v.x = *pointeur++;
	v.y = *pointeur++;
	return v;
}

inline auto pile_charge_vec3(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::vec3f v;
	v.x = *pointeur++;
	v.y = *pointeur++;
	v.z = *pointeur++;
	return v;
}

inline auto pile_charge_vec4(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::vec4f v;
	v.x = *pointeur++;
	v.y = *pointeur++;
	v.z = *pointeur++;
	v.w = *pointeur++;
	return v;
}

inline auto pile_charge_mat3(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::mat3x3f v;

	for (auto i = 0ul; i < 3; ++i) {
		for (auto j = 0ul; j < 3; ++j) {
			v[i][j] = *pointeur++;
		}
	}

	return v;
}

inline auto pile_charge_mat4(CompileuseGraphe::iterateur &pointeur)
{
	dls::math::mat4x4f v;

	for (auto i = 0ul; i < 4; ++i) {
		for (auto j = 0ul; j < 4; ++j) {
			v[i][j] = *pointeur++;
		}
	}

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

inline auto pile_charge_decimal_pointeur(
        CompileuseGraphe::iterateur const &debut,
        CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_decimal(pointeur);
}

inline auto pile_charge_entier_pointeur(
        CompileuseGraphe::iterateur const &debut,
        CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_entier(pointeur);
}

inline auto pile_charge_vec2_pointeur(
		CompileuseGraphe::iterateur const &debut,
		CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_vec2(pointeur);
}

inline auto pile_charge_vec3_pointeur(
        CompileuseGraphe::iterateur const &debut,
        CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_vec3(pointeur);
}

inline auto pile_charge_vec4_pointeur(
		CompileuseGraphe::iterateur const &debut,
		CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_vec4(pointeur);
}

inline auto pile_charge_mat3_pointeur(
		CompileuseGraphe::iterateur const &debut,
		CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_mat3(pointeur);
}

inline auto pile_charge_mat4_pointeur(
		CompileuseGraphe::iterateur const &debut,
		CompileuseGraphe::iterateur &courant)
{
	auto decalage = pile_charge_entier(courant);
	auto pointeur = debut + decalage;
	return pile_charge_mat4(pointeur);
}

inline void pile_stocke_entier(
		CompileuseGraphe::iterateur &pointeur,
		const int e)
{
	*pointeur++ = static_cast<float>(e);
}

inline void pile_stocke_decimal(
		CompileuseGraphe::iterateur &pointeur,
		const float d)
{
	*pointeur++ = d;
}

inline void pile_stocke_vec2(
		CompileuseGraphe::iterateur &pointeur,
		dls::math::vec2f const &v)
{
	*pointeur++ = v.x;
	*pointeur++ = v.y;
}

inline void pile_stocke_vec3(
		CompileuseGraphe::iterateur &pointeur,
		dls::math::vec3f const &v)
{
	*pointeur++ = v.x;
	*pointeur++ = v.y;
	*pointeur++ = v.z;
}

inline void pile_stocke_vec4(
		CompileuseGraphe::iterateur &pointeur,
		dls::math::vec4f const &v)
{
	*pointeur++ = v.x;
	*pointeur++ = v.y;
	*pointeur++ = v.z;
	*pointeur++ = v.w;
}

inline void pile_stocke_mat3(
		CompileuseGraphe::iterateur &pointeur,
		dls::math::mat3x3f const &v)
{
	for (auto i = 0ul; i < 3; ++i) {
		for (auto j = 0ul; j < 3; ++j) {
			*pointeur++ = v[i][j];
		}
	}
}

inline void pile_stocke_mat4(
		CompileuseGraphe::iterateur &pointeur,
		dls::math::mat4x4f const &v)
{
	for (auto i = 0ul; i < 4; ++i) {
		for (auto j = 0ul; j < 4; ++j) {
			*pointeur++ = v[i][j];
		}
	}
}

inline void pile_stocke_couleur(
		CompileuseGraphe::iterateur &pointeur,
		dls::phys::couleur32 const &c)
{
	*pointeur++ = c.r;
	*pointeur++ = c.v;
	*pointeur++ = c.b;
	*pointeur++ = c.a;
}
