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

#include "courbe_bezier.h"

#include <algorithm>
#include <cmath>

void ajoute_point_courbe(CourbeBezier &courbe, float x, float y)
{
	PointBezier point;
	point.co[POINT_CONTROLE1].x = x - 0.1f;
	point.co[POINT_CONTROLE1].y = y;
	point.co[POINT_CENTRE].x = x;
	point.co[POINT_CENTRE].y = y;
	point.co[POINT_CONTROLE2].x = x + 0.1f;
	point.co[POINT_CONTROLE2].y = y;

	courbe.points.push_back(point);

	std::sort(courbe.points.begin(), courbe.points.end(),
			  [](const PointBezier &p1, const PointBezier &p2)
	{
		return p1.co[POINT_CENTRE].x < p2.co[POINT_CENTRE].x;
	});

	courbe.extension_min.co[POINT_CENTRE].x = -1.0f;
	courbe.extension_min.co[POINT_CENTRE].y = courbe.points[0].co[POINT_CENTRE].y;
	courbe.extension_max.co[POINT_CENTRE].x = 2.0f;
	courbe.extension_max.co[POINT_CENTRE].y = courbe.points[courbe.points.size() - 1].co[POINT_CENTRE].y;

	construit_table_courbe(courbe);
}

void construit_table_courbe(CourbeBezier &courbe)
{
	const auto res_courbe = 32;
	const auto facteur = 1.0f / res_courbe;

	courbe.table.clear();
	courbe.table.reserve(res_courbe + 1);

	for (size_t i = 0; i < courbe.points.size() - 1; ++i) {
		const auto &p1 = courbe.points[i];
		const auto &p2 = courbe.points[i + 1];

		/* formule
		 *  x0 * (1.0 - t)^3
		 *  + 3 * x1 * t * (1.0 - t)^2
		 *  + 3 * x2 * t^2 * (1.0 - t)
		 *  + x3 * t^3
		 */

		const auto &x0 =     p1.co[POINT_CENTRE].x;
		const auto &y0 =     p1.co[POINT_CENTRE].y;
		const auto &x1 = 3 * p1.co[POINT_CONTROLE2].x;
		const auto &y1 = 3 * p1.co[POINT_CONTROLE2].y;
		const auto &x2 = 3 * p2.co[POINT_CONTROLE1].x;
		const auto &y2 = 3 * p2.co[POINT_CONTROLE1].y;
		const auto &x3 =     p2.co[POINT_CENTRE].x;
		const auto &y3 =     p2.co[POINT_CENTRE].y;

		courbe.table.push_back(Point{x0, y0});

		for (int i = 1; i <= res_courbe; ++i) {
			const auto fac_i = facteur * i;
			const auto mfac_i = 1.0f - fac_i;

			const auto p0x = x0                         * std::pow(mfac_i, 3.0f);
			const auto p0y = y0                         * std::pow(mfac_i, 3.0f);
			const auto p1x = x1 * fac_i                 * std::pow(mfac_i, 2.0f);
			const auto p1y = y1 * fac_i                 * std::pow(mfac_i, 2.0f);
			const auto p2x = x2 * std::pow(fac_i, 2.0f) * mfac_i;
			const auto p2y = y2 * std::pow(fac_i, 2.0f) * mfac_i;
			const auto p3x = x3 * std::pow(fac_i, 3.0f);
			const auto p3y = y3 * std::pow(fac_i, 3.0f);

			courbe.table.push_back(Point{p0x + p1x + p2x + p3x, p0y + p1y + p2y + p3y});
		}
	}
}
