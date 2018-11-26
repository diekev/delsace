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

#include "prim_points.h"

#include "outils/géométrie.h"

/* ************************************************************************** */

size_t PrimPoints::id = -1;

PrimPoints::PrimPoints()
{}

PrimPoints::PrimPoints(const PrimPoints &other)
    : Primitive(other)
{
	/* Copy points. */
	auto points = other.points();
	m_points.resize(points->size());

	for (auto i = 0ul; i < points->size(); ++i) {
		m_points[i] = (*points)[i];
	}
}

PrimPoints::~PrimPoints()
{
}

PointList *PrimPoints::points()
{
	return &m_points;
}

const PointList *PrimPoints::points() const
{
	return &m_points;
}

Primitive *PrimPoints::copy() const
{
	auto prim = new PrimPoints(*this);
	prim->tagUpdate();

	return prim;
}

size_t PrimPoints::typeID() const
{
	return PrimPoints::id;
}

void PrimPoints::computeBBox(dls::math::vec3f &min, dls::math::vec3f &max)
{
	calcule_boite_delimitation(m_points, m_min, m_max);

	min = m_min;
	max = m_max;
	m_dimensions = m_max - m_min;
}
