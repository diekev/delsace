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

#include "curves.h"
#include "draw_utils.h"

void draw(const ChaikinCurve &curve, QPainter &painter)
{
	painter.setPen(QPen(Qt::black, 5));
	draw_points(painter, curve.control_points);

	painter.setPen(QPen(Qt::black, 1));
	draw_lines(painter, curve.control_points, false);

	painter.setPen(QPen(Qt::red, 5));
	draw_points(painter, curve.chaikin_points);

	painter.setPen(QPen(Qt::red, 1));
	draw_lines(painter, curve.chaikin_points, false);
}

void draw(const SierpinskiCurve &curve, QPainter &painter)
{
	painter.setPen(QPen(Qt::black, 1));
	painter.drawRect(curve.center.x() - 100, curve.center.y() - 100, 200, 200);

	painter.setPen(QPen(Qt::red, 1));
	draw_lines(painter, curve.curve_points, true);
}

static QVector<QPoint> generate_chaikin_curve(QVector<QPoint> control_points)
{
	QVector<QPoint> chaikin_points;
	const auto num_points = control_points.size() - 1;

	for (auto i = 0; i < num_points; ++i) {
		const auto &p1 = control_points[i];
		const auto &p2 = control_points[i + 1];

		QPoint q1 = p1 * 3.0f / 4.0f + p2 * 1.0f / 4.0f;
		QPoint r1 = p1 * 1.0f / 4.0f + p2 * 3.0f / 4.0f;

		chaikin_points.push_back(q1);
		chaikin_points.push_back(r1);
	}

	return chaikin_points;
}

void generate_curve(ChaikinCurve &curve)
{
	if (curve.control_points.size() < 2) {
		return;
	}

	curve.chaikin_points = curve.control_points;

	for (int i = 0; i < curve.resolution; ++i) {
		curve.chaikin_points = generate_chaikin_curve(curve.chaikin_points);
	}
}

static void add_point(int x, int y, QPoint &center, QVector<QPoint> &points)
{
	center += QPoint(x, y);
	points.push_back(center);
}

static void sierpA(int level, int dist, QPoint &center, QVector<QPoint> &points);
static void sierpB(int level, int dist, QPoint &center, QVector<QPoint> &points);
static void sierpC(int level, int dist, QPoint &center, QVector<QPoint> &points);
static void sierpD(int level, int dist, QPoint &center, QVector<QPoint> &points);

static void sierpA(int level, int dist, QPoint &center, QVector<QPoint> &points)
{
	if (level > 0) {
		sierpA(level - 1, dist, center, points);
		add_point(+dist, +dist, center, points);
		sierpB(level - 1, dist, center, points);
		add_point(+2 * dist, 0, center, points);
		sierpD(level - 1, dist, center, points);
		add_point(+dist, -dist, center, points);
		sierpA(level - 1, dist, center, points);
	}
}

static void sierpB(int level, int dist, QPoint &center, QVector<QPoint> &points)
{
	if (level > 0) {
		sierpB(level - 1, dist, center, points);
		add_point(-dist, +dist, center, points);
		sierpC(level - 1, dist, center, points);
		add_point(0, +2 * dist, center, points);
		sierpA(level - 1, dist, center, points);
		add_point(+dist, +dist, center, points);
		sierpB(level - 1, dist, center, points);
	}
}

static void sierpC(int level, int dist, QPoint &center, QVector<QPoint> &points)
{
	if (level > 0) {
		sierpC(level - 1, dist, center, points);
		add_point(-dist, -dist, center, points);
		sierpD(level - 1, dist, center, points);
		add_point(-2 * dist, 0, center, points);
		sierpB(level - 1, dist, center, points);
		add_point(-dist, +dist, center, points);
		sierpC(level - 1, dist, center, points);
	}
}

static void sierpD(int level, int dist, QPoint &center, QVector<QPoint> &points)
{
	if (level > 0) {
		sierpD(level - 1, dist, center, points);
		add_point(+dist, -dist, center, points);
		sierpA(level - 1, dist, center, points);
		add_point(0, -2 * dist, center, points);
		sierpC(level - 1, dist, center, points);
		add_point(-dist, -dist, center, points);
		sierpD(level - 1, dist, center, points);
	}
}

void generate_curve(SierpinskiCurve &curve)
{
	curve.curve_points.clear();
	QPoint center = curve.center;

	int level = curve.resolution;
	int dist = 100;

	for (int i = 0; i < level; ++i) {
		dist /= 2;
	}

	center.setX(center.x() - 100 + dist);
	center.setY(center.y() - 100);

	sierpA(level, dist, center, curve.curve_points);
	add_point(+dist, +dist, center, curve.curve_points);

	sierpB(level, dist, center, curve.curve_points);
	add_point(-dist, +dist, center, curve.curve_points);

	sierpC(level, dist, center, curve.curve_points);
	add_point(-dist, -dist, center, curve.curve_points);

	sierpD(level, dist, center, curve.curve_points);
	add_point(+dist, -dist, center, curve.curve_points);
}
