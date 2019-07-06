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

bool entresecte_triangle(const Triangle &triangle, const Rayon &rayon, float &distance)
{
	constexpr auto epsilon = 0.000001f;

	auto const &vertex0 = triangle.v0;
	auto const &vertex1 = triangle.v1;
	auto const &vertex2 = triangle.v2;

	auto const &cote1 = vertex1 - vertex0;
	auto const &cote2 = vertex2 - vertex0;
	auto const &h = dls::math::produit_croix(rayon.direction, cote2);
	auto const angle = dls::math::produit_scalaire(cote1, h);

	if (angle > -epsilon && angle < epsilon) {
		return false;
	}

	auto const f = 1.0f / angle;
	auto const &s = Triangle::type_vec(rayon.origine) - vertex0;
	auto const angle_u = f * dls::math::produit_scalaire(s, h);

	if (angle_u < 0.0f || angle_u > 1.0f) {
		return false;
	}

	auto const q = dls::math::produit_croix(s, cote1);
	auto const angle_v = f * dls::math::produit_scalaire(rayon.direction, q);

	if (angle_v < 0.0f || angle_u + angle_v > 1.0f) {
		return false;
	}

	/* À cette étape on peut calculer t pour trouver le point d'entresection sur
	 * la ligne. */
	auto const t = f * dls::math::produit_scalaire(cote2, q);

	/* Entresection avec le rayon. */
	if (t > epsilon) {
		distance = t;
		return true;
	}

	/* Cela veut dire qu'il y a une entresection avec une ligne, mais pas avec
	 * le rayon. */
	return false;
}

long cherche_collision(const Corps *corps_collision, const Rayon &rayon_part, float &dist)
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
