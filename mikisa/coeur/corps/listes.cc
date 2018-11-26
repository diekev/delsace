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

#include "listes.h"

#include "corps.h"

/* ************************************************************************** */

Polygone *Polygone::construit(Corps *corps, int type_polygone, int nombre_sommets)
{
	Polygone *p = new Polygone;
	p->type = type_polygone;
	p->reserve_sommets(nombre_sommets);

	corps->ajoute_polygone(p);

	return p;
}

void Polygone::ajoute_sommet(int s)
{
	m_sommets.push_back(s);
}

void Polygone::reserve_sommets(size_t nombre)
{
	m_sommets.reserve(nombre);
}

size_t Polygone::nombre_sommets() const
{
	return m_sommets.size();
}

size_t Polygone::nombre_segments() const
{
	if (this->type == POLYGONE_FERME) {
		return this->nombre_sommets();
	}

	return this->nombre_sommets() - 1;
}

int Polygone::index_point(size_t i)
{
	return m_sommets[i];
}

/* ************************************************************************** */

ListePoints3D::~ListePoints3D()
{
	reinitialise();
}

void ListePoints3D::reinitialise()
{
	for (auto s : m_sommets) {
		delete s;
	}

	m_sommets.clear();
}

void ListePoints3D::redimensionne(const size_t nombre)
{
	m_sommets.resize(nombre);
}

void ListePoints3D::reserve(const size_t nombre)
{
	m_sommets.reserve(nombre);
}

const size_t ListePoints3D::taille() const
{
	return m_sommets.size();
}

void ListePoints3D::pousse(Point3D *s)
{
	m_sommets.push_back(s);
}

dls::math::vec3f ListePoints3D::point(size_t i) const
{
	auto p = m_sommets[i];
	return dls::math::vec3f(p->x, p->y, p->z);
}

ListePoints3D::plage_sommets ListePoints3D::points()
{
	return plage_sommets(m_sommets.begin(), m_sommets.end());
}

ListePoints3D::plage_const_sommets ListePoints3D::points() const
{
	return plage_const_sommets(m_sommets.cbegin(), m_sommets.cend());
}

/* ************************************************************************** */

ListePoints::~ListePoints()
{
	reinitialise();
}

void ListePoints::reinitialise()
{
	for (auto s : m_sommets) {
		delete s;
	}

	m_sommets.clear();
}

void ListePoints::redimensionne(const size_t nombre)
{
	m_sommets.resize(nombre);
}

void ListePoints::reserve(const size_t nombre)
{
	m_sommets.reserve(nombre);
}

const size_t ListePoints::taille() const
{
	return m_sommets.size();
}

void ListePoints::pousse(Sommet *s)
{
	m_sommets.push_back(s);
}

ListePoints::plage_sommets ListePoints::sommets()
{
	return plage_sommets(m_sommets.begin(), m_sommets.end());
}

ListePoints::plage_const_sommets ListePoints::sommets() const
{
	return plage_const_sommets(m_sommets.cbegin(), m_sommets.cend());
}

/* ************************************************************************** */

ListeSegments::~ListeSegments()
{
	reinitialise();
}

void ListeSegments::reinitialise()
{
	for (auto s : m_sommets) {
		delete s;
	}

	m_sommets.clear();
}

void ListeSegments::redimensionne(const size_t nombre)
{
	m_sommets.resize(nombre);
}

void ListeSegments::reserve(const size_t nombre)
{
	m_sommets.reserve(nombre);
}

const size_t ListeSegments::taille() const
{
	return m_sommets.size();
}

void ListeSegments::pousse(Arrete *s)
{
	m_sommets.push_back(s);
}

ListeSegments::plage_arretes ListeSegments::arretes()
{
	return plage_arretes(m_sommets.begin(), m_sommets.end());
}

ListeSegments::plage_const_arretes ListeSegments::arretes() const
{
	return plage_const_arretes(m_sommets.cbegin(), m_sommets.cend());
}

/* ************************************************************************** */

ListePolygones::~ListePolygones()
{
	reinitialise();
}

void ListePolygones::reinitialise()
{
	for (auto s : m_polygones) {
		delete s;
	}

	m_polygones.clear();
}

void ListePolygones::redimensionne(const size_t nombre)
{
	m_polygones.resize(nombre);
}

void ListePolygones::reserve(const size_t nombre)
{
	m_polygones.reserve(nombre);
}

const size_t ListePolygones::taille() const
{
	return m_polygones.size();
}

void ListePolygones::pousse(Polygone *s)
{
	m_polygones.push_back(s);
}

ListePolygones::plage_polys ListePolygones::polys()
{
	return plage_polys(m_polygones.begin(), m_polygones.end());
}

ListePolygones::plage_const_polys ListePolygones::polys() const
{
	return plage_const_polys(m_polygones.cbegin(), m_polygones.cend());
}
