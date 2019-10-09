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

#include "triangulation.hh"

#include "corps.h"

Triangle::Triangle(const Triangle::type_vec &v_0, const Triangle::type_vec &v_1, const Triangle::type_vec &v_2)
	: Triangle()
{
	v0 = v_0;
	v1 = v_1;
	v2 = v_2;
}

float calcule_aire(const Triangle &triangle)
{
	return calcule_aire(triangle.v0, triangle.v1, triangle.v2);
}

dls::tableau<Triangle> convertis_maillage_triangles(const Corps *corps_entree, GroupePrimitive *groupe)
{
	dls::tableau<Triangle> triangles;
	auto const points = corps_entree->points_pour_lecture();
	auto const prims  = corps_entree->prims();

	/* Convertis le maillage en triangles.
	 * Petit tableau pour comprendre le calcul du nombre de triangles.
	 * +----------------+------------------+
	 * | nombre sommets | nombre triangles |
	 * +----------------+------------------+
	 * | 3              | 1                |
	 * | 4              | 2                |
	 * | 5              | 3                |
	 * | 6              | 4                |
	 * | 7              | 5                |
	 * +----------------+------------------+
	 */

	auto nombre_triangles = 0l;

	iteratrice_index iter;

	if (groupe) {
		iter = iteratrice_index(groupe);
	}
	else {
		iter = iteratrice_index(prims->taille());
	}

	for (auto i : iter) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		nombre_triangles += poly->nombre_sommets() - 2;
	}

	triangles.reserve(nombre_triangles);

	for (auto ig : iter) {
		auto prim = prims->prim(ig);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (long i = 2; i < poly->nombre_sommets(); ++i) {
			Triangle triangle;

			triangle.v0 = points.point_local(poly->index_point(0));
			triangle.v1 = points.point_local(poly->index_point(i - 1));
			triangle.v2 = points.point_local(poly->index_point(i));
			triangle.index_orig = poly->index;

			triangles.pousse(triangle);
		}
	}

	return triangles;
}
