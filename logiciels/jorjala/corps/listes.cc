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

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "corps.h"
#include "sphere.hh"
#include "volume.hh"

/* ************************************************************************** */

void Polygone::ajoute_point(long idx_point, long idx_sommet)
{
	assert(idx_point >= 0);
	assert(idx_sommet >= 0);
	m_idx_points.ajoute(idx_point);
	m_idx_sommets.ajoute(idx_sommet);
}

void Polygone::reserve_sommets(long nombre)
{
	assert(nombre >= 0);
	m_idx_points.reserve(nombre);
	m_idx_sommets.reserve(nombre);
}

long Polygone::nombre_sommets() const
{
	return m_idx_points.taille();
}

long Polygone::nombre_segments() const
{
	if (this->type == type_polygone::FERME) {
		return this->nombre_sommets();
	}

	return this->nombre_sommets() - 1;
}

long Polygone::index_point(long i) const
{
	assert(i >= 0 && i < m_idx_points.taille());
	return m_idx_points[i];
}

long Polygone::index_sommet(long i) const
{
	assert(i >= 0 && i < m_idx_sommets.taille());
	return m_idx_sommets[i];
}

void Polygone::ajourne_index(long i, long j)
{
	assert(i >= 0);
	assert(j >= 0);
	m_idx_points[i] = j;
}

/* ************************************************************************** */

static void supprime_liste_points(ListePoints3D::type_liste *ptr)
{
	memoire::deloge("liste_point3d", ptr);
}

ListePoints3D::~ListePoints3D()
{
	reinitialise();
}

void ListePoints3D::reinitialise()
{
	if (m_sommets != nullptr) {
		if (!m_sommets.unique()) {
			m_sommets = RefPtr(memoire::loge<type_liste>("liste_point3d"), supprime_liste_points);
		}
		else {
			m_sommets->efface();
		}
	}
}

void ListePoints3D::redimensionne(const long nombre)
{
	assert(nombre >= 0);
	detache();
	m_sommets->redimensionne(nombre);
}

void ListePoints3D::reserve(long const nombre)
{
	assert(nombre >= 0);
	detache();
	m_sommets->reserve(nombre);
}

long ListePoints3D::taille() const
{
	if (m_sommets == nullptr) {
		return 0;
	}

	return m_sommets->taille();
}

void ListePoints3D::ajoute(const dls::math::vec3f &s)
{
	detache();
	m_sommets->ajoute(s);
}

dls::math::vec3f ListePoints3D::point(long i) const
{
	assert(i >= 0);
	return m_sommets->a(i);
}

void ListePoints3D::point(long i, dls::math::vec3f const &p)
{
	assert(i >= 0);
	detache();
	m_sommets->a(i) = p;
}

void ListePoints3D::detache()
{
	auto tmp = m_sommets.get();

	if (tmp != nullptr && !m_sommets.unique()) {
		m_sommets = RefPtr(memoire::loge<type_liste>("liste_point3d"), supprime_liste_points);
		m_sommets->reserve(tmp->taille());

		for (auto sommet : (*tmp)) {
			m_sommets->ajoute(sommet);
		}
	}
	else if (tmp == nullptr) {
		m_sommets = RefPtr(memoire::loge<type_liste>("liste_point3d"), supprime_liste_points);
	}
}

/* ************************************************************************** */

static void supprime_liste_prims(ListePrimitives::type_liste *ptr)
{
	memoire::deloge("liste_prims", ptr);
}

ListePrimitives::~ListePrimitives()
{
	reinitialise();
}

void ListePrimitives::reinitialise()
{
	if (m_primitives != nullptr) {
		if (!m_primitives.unique()) {
			m_primitives = RefPtr(memoire::loge<type_liste>("liste_prims"), supprime_liste_prims);
		}
		else {
			for (auto s : (*m_primitives)) {
				/* transtype pour pouvoir estimer la mémoire correctement */
				if (s->type_prim() == type_primitive::POLYGONE) {
					auto derivee = dynamic_cast<Polygone *>(s);
					memoire::deloge("Polygone", derivee);
				}
				else if (s->type_prim() == type_primitive::VOLUME) {
					auto derivee = dynamic_cast<Volume *>(s);
					memoire::deloge("Volume", derivee);
				}
				else if (s->type_prim() == type_primitive::SPHERE) {
					auto derivee = dynamic_cast<Sphere *>(s);
					memoire::deloge("Sphère", derivee);
				}
				else {
					/* au cas où */
					memoire::deloge("liste_prims", s);
				}
			}

			m_primitives->efface();
		}
	}
}

void ListePrimitives::redimensionne(long const nombre)
{
	assert(nombre >= 0);
	m_primitives->redimensionne(nombre);
}

void ListePrimitives::reserve(long const nombre)
{
	assert(nombre >= 0);
	detache();
	m_primitives->reserve(nombre);
}

long ListePrimitives::taille() const
{
	if (m_primitives == nullptr) {
		return 0;
	}

	return m_primitives->taille();
}

void ListePrimitives::ajoute(Primitive *s)
{
	detache();
	m_primitives->ajoute(s);
}

Primitive *ListePrimitives::prim(long index) const
{
	return m_primitives->a(index);
}

void ListePrimitives::prim(long i, Primitive *p)
{
	detache();
	(*m_primitives)[i] = p;
}

void ListePrimitives::detache()
{
	auto tmp = m_primitives.get();

	if (tmp != nullptr && !m_primitives.unique()) {
		m_primitives = RefPtr(memoire::loge<type_liste>("liste_prims"), supprime_liste_prims);
		m_primitives->reserve(tmp->taille());

		for (auto prim : (*tmp)) {
			if (prim->type_prim() == type_primitive::POLYGONE) {
				auto polygone = dynamic_cast<Polygone *>(prim);

				auto p = memoire::loge<Polygone>("Polygone");
				p->type = polygone->type;
				p->reserve_sommets(polygone->nombre_sommets());
				p->index = polygone->index;

				for (long i = 0; i < polygone->nombre_sommets(); ++i) {
					p->ajoute_point(polygone->index_point(i), polygone->index_sommet(i));
				}

				m_primitives->ajoute(p);
			}
			else if (prim->type_prim() == type_primitive::VOLUME) {
				auto volume = dynamic_cast<Volume *>(prim);
				auto grille = static_cast<wlk::base_grille_3d *>(nullptr);

				if (volume->grille) {
					grille = volume->grille->copie();
				}

				auto nouveau_volume = memoire::loge<Volume>("Volume", grille);
				nouveau_volume->index = volume->index;
				m_primitives->ajoute(nouveau_volume);
			}
			else if (prim->type_prim() == type_primitive::SPHERE) {
				auto sphere = dynamic_cast<Sphere *>(prim);

				auto nvl_volume = memoire::loge<Sphere>("Sphère", sphere->idx_point, sphere->rayon);
				nvl_volume->index = sphere->index;
				m_primitives->ajoute(nvl_volume);
			}
		}
	}
	else if (tmp == nullptr) {
		m_primitives = RefPtr(memoire::loge<type_liste>("liste_prims"), supprime_liste_prims);
	}
}
