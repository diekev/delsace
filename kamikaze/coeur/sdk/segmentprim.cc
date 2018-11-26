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

#include "segmentprim.h"

#include "outils/géométrie.h"

/* ************************************************************************** */

size_t SegmentPrim::id = -1;

SegmentPrim::SegmentPrim()
{}

SegmentPrim::SegmentPrim(const SegmentPrim &other)
    : Primitive(other)
{
	/* Copy points. */
	auto points = other.points();
	m_points.resize(points->size());

	for (auto i = 0ul; i < points->size(); ++i) {
		m_points[i] = (*points)[i];
	}

	/* Copy edges. */
	auto edges = other.edges();
	m_edges.resize(edges->size());

	for (auto i = 0ul; i < edges->size(); ++i) {
		m_edges[i] = (*edges)[i];
	}
}

SegmentPrim::~SegmentPrim()
{
}

PointList *SegmentPrim::points()
{
	return &m_points;
}

const PointList *SegmentPrim::points() const
{
	return &m_points;
}

EdgeList *SegmentPrim::edges()
{
	return &m_edges;
}

const EdgeList *SegmentPrim::edges() const
{
	return &m_edges;
}

Primitive *SegmentPrim::copy() const
{
	auto prim = new SegmentPrim(*this);
	prim->tagUpdate();

	return prim;
}

size_t SegmentPrim::typeID() const
{
	return SegmentPrim::id;
}

void SegmentPrim::computeBBox(dls::math::vec3f &min, dls::math::vec3f &max)
{
	calcule_boite_delimitation(m_points, m_min, m_max);

	min = m_min;
	max = m_max;
	m_dimensions = m_max - m_min;
}
