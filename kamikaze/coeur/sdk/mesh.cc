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

#include "mesh.h"

#include "outils/géométrie.h"

/* ************************************************************************** */

size_t Mesh::id = -1;

Mesh::Mesh()
    : Primitive()
{
	add_attribute("normal", ATTR_TYPE_VEC3, 0);
	m_need_update = true;
}

Mesh::Mesh(const Mesh &other)
    : Primitive(other)
{
	/* Copy points. */
	auto points = other.points();
	m_point_list.resize(points->size());

	for (auto i = 0ul; i < points->size(); ++i) {
		m_point_list[i] = (*points)[i];
	}

	/* Copy points. */
	auto polys = other.polys();
	m_poly_list.resize(polys->size());

	for (auto i = 0ul; i < polys->size(); ++i) {
		m_poly_list[i] = (*polys)[i];
	}
}

Mesh::~Mesh()
{
}

PointList *Mesh::points()
{
	return &m_point_list;
}

const PointList *Mesh::points() const
{
	return &m_point_list;
}

PolygonList *Mesh::polys()
{
	return &m_poly_list;
}

const PolygonList *Mesh::polys() const
{
	return &m_poly_list;
}

void Mesh::update()
{
	if (m_need_update) {
		computeBBox(m_min, m_max);
		m_need_update = false;
	}
}

Primitive *Mesh::copy() const
{
	auto mesh = new Mesh(*this);
	mesh->tagUpdate();

	return mesh;
}

size_t Mesh::typeID() const
{
	return Mesh::id;
}

void Mesh::computeBBox(dls::math::vec3f &min, dls::math::vec3f &max)
{
	calcule_boite_delimitation(m_point_list, m_min, m_max);

	min = m_min;
	max = m_max;
	m_dimensions = m_max - m_min;
}
