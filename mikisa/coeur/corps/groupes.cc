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

#include "groupes.h"

/* ************************************************************************** */

void GroupePoint::ajoute_point(int index_point)
{
	this->m_points.push_back(index_point);
}

void GroupePoint::reserve(const size_t nombre)
{
	this->m_points.reserve(nombre);
}

GroupePoint::plage_points GroupePoint::index()
{
	return plage_points(m_points.begin(), m_points.end());
}

GroupePoint::plage_points_const GroupePoint::index() const
{
	return plage_points_const(m_points.cbegin(), m_points.cend());
}

/* ************************************************************************** */

void GroupePolygone::ajoute_primitive(int index_poly)
{
	this->m_polygones.push_back(index_poly);
}

void GroupePolygone::reserve(const size_t nombre)
{
	this->m_polygones.reserve(nombre);
}

GroupePolygone::plage_points GroupePolygone::index()
{
	return plage_points(m_polygones.begin(), m_polygones.end());
}

GroupePolygone::plage_points_const GroupePolygone::index() const
{
	return plage_points_const(m_polygones.cbegin(), m_polygones.cend());
}
