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

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/tableau.hh"

template <typename T>
class Grille {
	dls::tableau<T> m_donnees = {};

	dls::math::vec3<long> m_res = dls::math::vec3<long>(0l, 0l, 0l);
	long m_nombre_voxels = 0;

	T m_arriere_plan = T(0);

	long calcul_index(long x, long y, long z) const
	{
		return x + (y + z * m_res[1]) * m_res[0];
	}

	bool hors_des_limites(long x, long y, long z) const
	{
		if (x < 0 || x >= m_res[0]) {
			return true;
		}

		if (y < 0 || y >= m_res[1]) {
			return true;
		}

		if (z < 0 || z >= m_res[2]) {
			return true;
		}

		return false;
	}

public:
	Grille() = default;

	void initialise(long res_x, long res_y, long res_z)
	{
		m_res[0] = res_x;
		m_res[1] = res_y;
		m_res[2] = res_z;

		m_nombre_voxels = res_x * res_y * res_z;

		m_donnees.redimensionne(m_nombre_voxels);
		std::fill(m_donnees.debut(), m_donnees.fin(), T(0));
	}

	dls::math::vec3<long> resolution() const
	{
		return m_res;
	}

	T valeur(long index) const
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T valeur(long x, long y, long z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[calcul_index(x, y, z)];
	}

	void valeur(long index, T v)
	{
		if (index >= m_nombre_voxels) {
			return;
		}

		m_donnees[index] = v;
	}

	void valeur(long x, long y, long z, T v)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[calcul_index(x, y, z)] = v;
	}

	void copie(Grille<T> const &grille)
	{
		for (long i = 0; i < m_nombre_voxels; ++i) {
			m_donnees[i] = grille.m_donnees[i];
		}
	}

	void arriere_plan(T const &v)
	{
		m_arriere_plan = v;
	}

	void *donnees() const
	{
		return m_donnees.donnees();
	}

	size_t taille_octet() const
	{
		return static_cast<size_t>(m_nombre_voxels) * sizeof(T);
	}
};
