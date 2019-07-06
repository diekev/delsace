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

#include <set>

#include <delsace/math/outils.hh>
#include <delsace/math/vecteur.hh>

class GrilleParticules {
	dls::math::point3d m_min{};
	dls::math::point3d m_max{};
	dls::math::point3d m_dim{};
	size_t m_res_x{};
	size_t m_res_y{};
	size_t m_res_z{};
	double m_distance{};

	std::vector<std::vector<dls::math::vec3f>> m_grille{};

public:
	explicit GrilleParticules(dls::math::point3d const &min, dls::math::point3d const &max, float distance);

	void ajoute(dls::math::vec3f const &position);

	bool verifie_distance_minimal(dls::math::vec3f const &point, float distance);

	bool triangle_couvert(
			dls::math::vec3f const &v0,
			dls::math::vec3f const &v1,
			dls::math::vec3f const &v2,
			const float radius);

	size_t calcul_index_pos(dls::math::vec3f const &point);
};
