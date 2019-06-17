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

#include "curves.h"
#include "polygon.h"

enum {
	MODE_CHAIKIN    = 0,
	MODE_SIERPINSKI = 1,
	MODE_POLYGON    = 2,
};

class Scene : public QObject {
	Q_OBJECT

	QVector<ChaikinCurve> m_chaikin_curves;
	ChaikinCurve *m_cur_curve;

	QVector<SierpinskiCurve> m_sierpenski_curves;
	SierpinskiCurve *m_cur_sierpenski;

	QVector<Polygon> m_polygons;
	Polygon *m_cur_polygon;

	int m_mode = MODE_CHAIKIN;

public:
	Scene();

	const QVector<ChaikinCurve> &chaikinCurves() const;
	const QVector<SierpinskiCurve> &sierpenskiCurves() const;
	const QVector<Polygon> &polygons() const;

	void addCurvePoint(const QPoint &pos);

Q_SIGNALS:
	void redraw();

public Q_SLOTS:
	void addChaikinCurve();
	void closeShape();
	void addSierpinskiCurve(const QPoint &pos);
	void changeMode(int mode);
	void setCurveResolution(int res);
};
