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

		const auto &x1 = p1.co[POINT_CENTRE].x;
		const auto &y1 = p1.co[POINT_CENTRE].y;
		const auto &x_pt2 = p1.co[POINT_CONTROLE2].x;
		const auto &y_pt2 = p1.co[POINT_CONTROLE2].y;
		const auto &x2 = p2.co[POINT_CENTRE].x;
		const auto &y2 = p2.co[POINT_CENTRE].y;
		const auto &x_pt1 = p2.co[POINT_CONTROLE1].x;
		const auto &y_pt1 = p2.co[POINT_CONTROLE1].y;

		courbe.table.push_back(Point{x1, y1});

		for (int i = 1; i <= res_courbe; ++i) {
			const auto fac_i = facteur * i;
			const auto mfac_i = 1.0f - fac_i;

			/* centre -> pt2 */
			const auto x_c_pt2 = mfac_i * x1 + fac_i * x_pt2;
			const auto y_c_pt2 = mfac_i * y1 + fac_i * y_pt2;

			/* pt2 -> pt1 */
			const auto x_pt2_pt1 = mfac_i * x_pt2 + fac_i * x_pt1;
			const auto y_pt2_pt1 = mfac_i * y_pt2 + fac_i * y_pt1;

			/* pt1 -> centre */
			const auto x_pt1_c = mfac_i * x_pt1 + fac_i * x2;
			const auto y_pt1_c = mfac_i * y_pt1 + fac_i * y2;

			/* c_pt2 -> pt2_pt1 */
			const auto x_c_pt1 = mfac_i * x_c_pt2 + fac_i * x_pt2_pt1;
			const auto y_c_pt1 = mfac_i * y_c_pt2 + fac_i * y_pt2_pt1;

			/* pt2_pt1 -> pt1_c */
			const auto x_pt2_c = mfac_i * x_pt2_pt1 + fac_i * x_pt1_c;
			const auto y_pt2_c = mfac_i * y_pt2_pt1 + fac_i * y_pt1_c;

			const auto xt2 = mfac_i * x_c_pt1 + fac_i * x_pt2_c;
			const auto yt2 = mfac_i * y_c_pt1 + fac_i * y_pt2_c;

			courbe.table.push_back(Point{xt2, yt2});
		}
	}
}
