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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "polygon.h"

#include "draw_utils.h"

static float area(const QVector<QPoint> &vertices)
{
	auto area = float(0);

	/* "start" at the last vertex since we need to loop around the vertices */
	for (size_t i = 0, ie = vertices.size(), j = ie - 1; i < ie; j = i++) {
		const auto &v1 = vertices[j];
		const auto &v2 = vertices[i];

		area += (v1.x() * v2.y() - v1.y() * v2.x());
		j = i;
	}

	area *= 0.5f;
	return area;
}

static QPoint centroid(const QVector<QPoint> &vertices, const float area)
{
	auto cent = QPoint(0, 0);

	/* "start" at the last vertex since we need to loop around the vertices */
	for (size_t i = 0, ie = vertices.size(), j = ie - 1; i < ie; j = i++) {
		const auto &v1 = vertices[j];
		const auto &v2 = vertices[i];
		auto c = v1;
		c += v2;
		c *= (v1.x() * v2.y() - v2.x() * v1.y());
		cent += c;
	}

	cent *= (float(1) / (float(6) * area));
	return cent;
}

void generate_centroid(Polygon &polygon)
{
	polygon.centroid = centroid(polygon.vertices, area(polygon.vertices));
}

void draw(const Polygon &polygon, QPainter &painter)
{
	painter.setPen(QPen(Qt::black, 5));
	draw_points(painter, polygon.vertices);

	painter.setPen(QPen(Qt::black, 1));
	draw_lines(painter, polygon.vertices, true);

	painter.setPen(QPen(Qt::red, 5));
	painter.drawPoint(polygon.centroid);
}
