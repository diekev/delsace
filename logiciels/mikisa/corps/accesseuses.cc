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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "accesseuses.hh"

#include "corps.h"

AccesseusePointLecture::AccesseusePointLecture(const ListePoints3D &pnts, const math::transformation &transform)
	: m_points(pnts)
	, m_transformation(transform)
{}

type_point AccesseusePointLecture::point_local(long idx) const
{
	return m_points.point(idx);
}

type_point AccesseusePointLecture::point_monde(long idx) const
{
	auto p = m_points.point(idx);
	auto pos_monde_d = m_transformation(dls::math::point3d(p));
	return dls::math::converti_type_vecteur<float>(pos_monde_d);
}

long AccesseusePointLecture::taille() const
{
	return m_points.taille();
}

AccesseusePointEcriture::AccesseusePointEcriture(Corps &corps, ListePoints3D &pnts, const math::transformation &transform)
	: m_corps(corps)
	, m_points(pnts)
	, m_transformation(transform)
{}

type_point AccesseusePointEcriture::point_local(long idx) const
{
	return m_points.point(idx);
}

type_point AccesseusePointEcriture::point_monde(long idx) const
{
	auto p = m_points.point(idx);
	auto pos_monde_d = m_transformation(dls::math::point3d(p));
	return dls::math::converti_type_vecteur<float>(pos_monde_d);
}

long AccesseusePointEcriture::ajoute_point(const type_point &pnt)
{
	auto decalage = m_points.taille();
	m_points.pousse(pnt);
	m_corps.redimensionne_attributs(portee_attr::POINT);

	return decalage;
}

long AccesseusePointEcriture::ajoute_point(float x, float y, float z)
{
	return ajoute_point(type_point(x, y, z));
}

void AccesseusePointEcriture::point(long idx, const type_point &pnt)
{
	m_points.point(idx, pnt);
}

void AccesseusePointEcriture::redimensionne(long nombre)
{
	m_points.redimensionne(nombre);
}

void AccesseusePointEcriture::reserve(long nombre)
{
	m_points.reserve(nombre);
}

long AccesseusePointEcriture::taille() const
{
	return m_points.taille();
}
