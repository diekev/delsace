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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "géométrie.h"

#include "../attribute.h"
#include "../geomlists.h"

#include "bibliotheques/outils/parallelisme.h"

void calcule_normales(
		const PointList &points,
		const PolygonList &polygones,
		Attribute &normales,
		bool flip)
{
	boucle_parallele(tbb::blocked_range<size_t>(0ul, polygones.size()),
				 [&](const tbb::blocked_range<size_t> &r)
	{
		for (auto i = r.begin(), ie = r.end(); i < ie ; ++i) {
			auto const &quad = polygones[i];

			auto const v0 = points[static_cast<size_t>(quad[0])];
			auto const v1 = points[static_cast<size_t>(quad[1])];
			auto const v2 = points[static_cast<size_t>(quad[2])];

			auto const normal = normale_triangle(v0, v1, v2);

			normales.vec3(static_cast<size_t>(quad[0]), normales.vec3(static_cast<size_t>(quad[0])) + normal);
			normales.vec3(static_cast<size_t>(quad[1]), normales.vec3(static_cast<size_t>(quad[1])) + normal);
			normales.vec3(static_cast<size_t>(quad[2]), normales.vec3(static_cast<size_t>(quad[2])) + normal);

			if (quad[3] != static_cast<int>(INVALID_INDEX)) {
				normales.vec3(static_cast<size_t>(quad[3]), normales.vec3(static_cast<size_t>(quad[3])) + normal);
			}
		}
	});

	if (flip) {
		for (size_t i = 0, ie = points.size(); i < ie ; ++i) {
			normales.vec3(i, dls::math::normalise(normales.vec3(i)));
		}
	}
	else {
		for (size_t i = 0, ie = points.size(); i < ie ; ++i) {
			normales.vec3(i, -dls::math::normalise(normales.vec3(i)));
		}
	}
}

void calcule_boite_delimitation(
		const PointList &points,
		dls::math::vec3f &min,
		dls::math::vec3f &max)
{
	for (size_t i = 0, ie = points.size(); i < ie; ++i) {
		auto vert = points[i];

		if (vert.x < min.x) {
			min.x = vert.x;
		}
		else if (vert.x > max.x) {
			max.x = vert.x;
		}

		if (vert.y < min.y) {
			min.y = vert.y;
		}
		else if (vert.y > max.y) {
			max.y = vert.y;
		}

		if (vert.z < min.z) {
			min.z = vert.z;
		}
		else if (vert.z > max.z) {
			max.z = vert.z;
		}
	}
}
