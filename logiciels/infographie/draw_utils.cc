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

#include "draw_utils.h"

void draw_points(QPainter &painter, const QVector<QPoint> &points)
{
	painter.drawPoints(points.data(), points.size());
}

void draw_lines(QPainter &painter, const QVector<QPoint> &points, bool close)
{
	const auto num_points = points.size() - 1;

	for (auto i = 0; i < num_points; ++i) {
		painter.drawLine(points[i], points[i + 1]);
	}

	if (close) {
		painter.drawLine(points[num_points], points[0]);
	}
}
