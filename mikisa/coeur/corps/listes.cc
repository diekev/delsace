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

Polygone *Polygone::construit(Corps *corps, type_polygone type_poly, long nombre_sommets)
{
	Polygone *p = new Polygone;
	p->type = type_poly;
	p->reserve_sommets(nombre_sommets);

	corps->ajoute_primitive(p);

	return p;
}

void Polygone::ajoute_sommet(long sommet)
{
	assert(sommet >= 0);
	m_sommets.push_back(static_cast<size_t>(sommet));
}

void Polygone::reserve_sommets(long nombre)
{
	assert(nombre >= 0);
	m_sommets.reserve(static_cast<size_t>(nombre));
}

long Polygone::nombre_sommets() const
{
	return static_cast<long>(m_sommets.size());
}

long Polygone::nombre_segments() const
{
	if (this->type == type_polygone::FERME) {
		return this->nombre_sommets();
	}

	return this->nombre_sommets() - 1;
}

long Polygone::index_point(long i)
{
	assert(i >= 0);
	return static_cast<long>(m_sommets[static_cast<size_t>(i)]);
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

void ListePoints3D::redimensionne(const long nombre)
{
	assert(nombre >= 0);
	m_sommets.resize(static_cast<size_t>(nombre));
}

void ListePoints3D::reserve(long const nombre)
{
	assert(nombre >= 0);
	m_sommets.reserve(static_cast<size_t>(nombre));
}

long ListePoints3D::taille() const
{
	return static_cast<long>(m_sommets.size());
}

void ListePoints3D::pousse(Point3D *s)
{
	m_sommets.push_back(s);
}

dls::math::vec3f ListePoints3D::point(long i) const
{
	assert(i >= 0);
	auto p = m_sommets[static_cast<size_t>(i)];
	return dls::math::vec3f(p->x, p->y, p->z);
}

void ListePoints3D::point(long i, dls::math::vec3f const &p) const
{
	assert(i >= 0);
	auto s = m_sommets[static_cast<size_t>(i)];
	s->x = p.x;
	s->y = p.y;
	s->z = p.z;
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

void ListePoints::redimensionne(long const nombre)
{
	assert(nombre >= 0);
	m_sommets.resize(static_cast<size_t>(nombre));
}

void ListePoints::reserve(long const nombre)
{
	assert(nombre >= 0);
	m_sommets.reserve(static_cast<size_t>(nombre));
}

long ListePoints::taille() const
{
	return static_cast<long>(m_sommets.size());
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

void ListeSegments::redimensionne(long const nombre)
{
	assert(nombre >= 0);
	m_sommets.resize(static_cast<size_t>(nombre));
}

void ListeSegments::reserve(long const nombre)
{
	assert(nombre >= 0);
	m_sommets.reserve(static_cast<size_t>(nombre));
}

long ListeSegments::taille() const
{
	return static_cast<long>(m_sommets.size());
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

ListePrimitives::~ListePrimitives()
{
	reinitialise();
}

void ListePrimitives::reinitialise()
{
	for (auto s : m_primitives) {
		delete s;
	}

	m_primitives.clear();
}

void ListePrimitives::redimensionne(long const nombre)
{
	assert(nombre >= 0);
	m_primitives.resize(static_cast<size_t>(nombre));
}

void ListePrimitives::reserve(long const nombre)
{
	assert(nombre >= 0);
	m_primitives.reserve(static_cast<size_t>(nombre));
}

long ListePrimitives::taille() const
{
	return static_cast<long>(m_primitives.size());
}

void ListePrimitives::pousse(Primitive *s)
{
	m_primitives.push_back(s);
}

Primitive *ListePrimitives::prim(size_t index) const
{
	return m_primitives[index];
}

ListePrimitives::plage_prims ListePrimitives::prims()
{
	return plage_prims(m_primitives.begin(), m_primitives.end());
}

ListePrimitives::plage_const_prims ListePrimitives::prims() const
{
	return plage_const_prims(m_primitives.cbegin(), m_primitives.cend());
}
