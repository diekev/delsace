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

#include "collision.hh"

#include "corps.h"
#include "triangulation.hh"

bool entresecte_triangle(
		Triangle const &triangle,
		dls::phys::rayonf const &rayon,
		float &distance)
{
	return entresecte_triangle(
				dls::math::point3f(triangle.v0),
				dls::math::point3f(triangle.v1),
				dls::math::point3f(triangle.v2),
				rayon,
				distance);
}

long cherche_collision(
		Corps const *corps_collision,
		dls::phys::rayonf const &rayon_part,
		float &dist)
{
	auto const prims_collision = corps_collision->prims();
	auto const points_collision = corps_collision->points();

	/* À FAIRE : collision particules
	 * - structure accélération
	 */
	for (auto ip = 0; ip < prims_collision->taille(); ++ip) {
		auto prim = prims_collision->prim(ip);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto const &v0 = points_collision->point(poly->index_point(0));
			auto const &v1 = points_collision->point(poly->index_point(j - 1));
			auto const &v2 = points_collision->point(poly->index_point(j));

			auto const &v0_d = corps_collision->transformation(dls::math::point3d(v0));
			auto const &v1_d = corps_collision->transformation(dls::math::point3d(v1));
			auto const &v2_d = corps_collision->transformation(dls::math::point3d(v2));

			auto triangle = Triangle{};
			triangle.v0 = dls::math::vec3f(
						static_cast<float>(v0_d.x),
						static_cast<float>(v0_d.y),
						static_cast<float>(v0_d.z));
			triangle.v1 = dls::math::vec3f(
						static_cast<float>(v1_d.x),
						static_cast<float>(v1_d.y),
						static_cast<float>(v1_d.z));
			triangle.v2 = dls::math::vec3f(
						static_cast<float>(v2_d.x),
						static_cast<float>(v2_d.y),
						static_cast<float>(v2_d.z));

			if (entresecte_triangle(triangle, rayon_part, dist)) {
				return static_cast<long>(prim->index);
			}
		}
	}
	return -1;
}
