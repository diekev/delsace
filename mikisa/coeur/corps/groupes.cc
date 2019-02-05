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

#include <cassert>

/* ************************************************************************** */

void GroupePoint::ajoute_point(size_t index_point)
{
	this->m_points.push_back(index_point);
}

void GroupePoint::reserve(long const nombre)
{
	assert(nombre >= 0);
	this->m_points.reserve(static_cast<size_t>(nombre));
}

void GroupePoint::reinitialise()
{
	m_points.clear();
}

long GroupePoint::taille() const
{
	return static_cast<long>(m_points.size());
}

bool GroupePoint::contiens(size_t index_point) const
{
	for (auto const i : m_points) {
		if (i == index_point) {
			return true;
		}
	}

	return false;
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

void GroupePrimitive::ajoute_primitive(size_t index_poly)
{
	this->m_primitives.push_back(index_poly);
}

void GroupePrimitive::reserve(long const nombre)
{
	assert(nombre >= 0);
	this->m_primitives.reserve(static_cast<size_t>(nombre));
}

void GroupePrimitive::reinitialise()
{
	m_primitives.clear();
}

long GroupePrimitive::taille() const
{
	return static_cast<long>(m_primitives.size());
}

GroupePrimitive::plage_prims GroupePrimitive::index()
{
	return plage_prims(m_primitives.begin(), m_primitives.end());
}

GroupePrimitive::plage_prims_const GroupePrimitive::index() const
{
	return plage_prims_const(m_primitives.cbegin(), m_primitives.cend());
}
