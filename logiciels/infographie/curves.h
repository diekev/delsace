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

#pragma once

#include <QPainter>
#include <QPoint>
#include <QVector>

struct ChaikinCurve {
	QVector<QPoint> control_points;
	QVector<QPoint> chaikin_points;
	int resolution = 3;
};

struct SierpinskiCurve {
	QPoint center;
	QVector<QPoint> curve_points;
	int resolution = 1;
};

void draw(const ChaikinCurve &curve, QPainter &painter);
void draw(const SierpinskiCurve &curve, QPainter &painter);

void generate_curve(ChaikinCurve &curve);
void generate_curve(SierpinskiCurve &curve);
