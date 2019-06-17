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
#include "scene.h"

Scene::Scene()
    : m_cur_curve(nullptr)
    , m_cur_sierpenski(nullptr)
    , m_cur_polygon(nullptr)
{
	addChaikinCurve();
}

const QVector<ChaikinCurve> &Scene::chaikinCurves() const
{
	return m_chaikin_curves;
}

const QVector<SierpinskiCurve> &Scene::sierpenskiCurves() const
{
	return m_sierpenski_curves;
}

const QVector<Polygon> &Scene::polygons() const
{
	return m_polygons;
}

void Scene::addCurvePoint(const QPoint &pos)
{
	switch (m_mode) {
		case MODE_CHAIKIN:
			m_cur_curve->control_points.push_back(pos);
			generate_curve(*m_cur_curve);
			break;
		case MODE_SIERPINSKI:
			addSierpinskiCurve(pos);
			break;
		case MODE_POLYGON:
			if (m_cur_polygon == nullptr) {
				m_polygons.push_back(Polygon());
				m_cur_polygon = &m_polygons.back();
			}

			m_cur_polygon->vertices.push_back(pos);
			generate_centroid(*m_cur_polygon);
			break;
	}
}

void Scene::addChaikinCurve()
{
	m_chaikin_curves.push_back(ChaikinCurve());
	m_cur_curve = &m_chaikin_curves.back();
}

void Scene::closeShape()
{
	switch (m_mode) {
		case MODE_CHAIKIN:
			m_cur_curve->control_points.push_back(m_cur_curve->control_points[0]);
			m_cur_curve->control_points.push_back(m_cur_curve->control_points[1]);
			generate_curve(*m_cur_curve);
			break;
		case MODE_POLYGON:
			m_cur_polygon->vertices.push_back(m_cur_polygon->vertices[0]);
			generate_centroid(*m_cur_polygon);
			break;
		default:
			break;
	}

	Q_EMIT redraw();
}

void Scene::addSierpinskiCurve(const QPoint &pos)
{
	m_sierpenski_curves.push_back(SierpinskiCurve());
	m_cur_sierpenski = &m_sierpenski_curves.back();
	m_cur_sierpenski->center = pos;
	generate_curve(*m_cur_sierpenski);
}

void Scene::changeMode(int mode)
{
	m_mode = mode;
}

void Scene::setCurveResolution(int res)
{
	switch (m_mode) {
		case MODE_CHAIKIN:
			m_cur_curve->resolution = res;
			generate_curve(*m_cur_curve);
			break;
		case MODE_SIERPINSKI:
			m_cur_sierpenski->resolution = res;
			generate_curve(*m_cur_sierpenski);
			break;
		default:
			break;
	}

	Q_EMIT redraw();
}
