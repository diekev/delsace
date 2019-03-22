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

#include "primitive.hh"

#include <delsace/math/vecteur.hh>
#include <vector>

#include "../logeuse_memoire.hh"

enum class type_volume : char {
	SCALAIRE,
	VECTOR,
};

/* ************************************************************************** */

class BaseGrille {
protected:
	dls::math::vec3<size_t> m_res = dls::math::vec3<size_t>(0ul, 0ul, 0ul);
	size_t m_nombre_voxels = 0;

	size_t calcul_index(size_t x, size_t y, size_t z) const;

	bool hors_des_limites(size_t x, size_t y, size_t z) const;

public:
	BaseGrille() = default;
	virtual ~BaseGrille() = default;

	dls::math::vec3<size_t> resolution() const;

	virtual BaseGrille *copie() const = 0;
	virtual type_volume type() const = 0;

	dls::math::vec3f min{};
	dls::math::vec3f max{};
	dls::math::vec3f dim{};
};

/* ************************************************************************** */

template <typename T>
class Grille : public BaseGrille {
	std::vector<T> m_donnees = {};

	T m_arriere_plan = T(0);
	T pad{};

public:
	Grille() = default;

	void initialise(size_t res_x, size_t res_y, size_t res_z)
	{
		m_res[0] = res_x;
		m_res[1] = res_y;
		m_res[2] = res_z;

		m_nombre_voxels = res_x * res_y * res_z;

		m_donnees.resize(m_nombre_voxels);
		std::fill(m_donnees.begin(), m_donnees.end(), T(0));
	}

	T valeur(size_t index) const
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T valeur(size_t x, size_t y, size_t z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[calcul_index(x, y, z)];
	}

	void valeur(size_t index, T v)
	{
		if (index >= m_nombre_voxels) {
			return;
		}

		m_donnees[index] = v;
	}

	void valeur(size_t x, size_t y, size_t z, T v)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[calcul_index(x, y, z)] = v;
	}

	void copie_donnees(const Grille<T> &grille)
	{
		for (size_t i = 0; i < m_nombre_voxels; ++i) {
			m_donnees[i] = grille.m_donnees[i];
		}
	}

	void arriere_plan(const T &v)
	{
		m_arriere_plan = v;
	}

	void const *donnees() const
	{
		return m_donnees.data();
	}

	size_t taille_octet() const
	{
		return m_nombre_voxels * sizeof(T);
	}

	BaseGrille *copie() const override
	{
		auto grille = memoire::loge<Grille<T>>();
		grille->m_res = this->m_res;
		grille->m_nombre_voxels = this->m_nombre_voxels;
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_donnees = this->m_donnees;

		return grille;
	}

	type_volume type() const override
	{
		return type_volume::SCALAIRE;
	}
};

/* ************************************************************************** */

class Volume final : public Primitive {
public:
	BaseGrille *grille = nullptr;

	~Volume();

	type_primitive type_prim() const;
};
