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

#include "maillage.h"

#include "../attribut.h"

/* ************************************************************************** */

Maillage::Maillage()
	: Corps()
{
	this->nom = "maillage";
	this->type = CORPS_MAILLAGE;
}

Maillage::~Maillage()
{
	reinitialise();
}

void Maillage::ajoute_sommet(const dls::math::vec3f &coord)
{
	auto sommet = new Sommet();
	sommet->pos = coord;
	sommet->index = m_sommets.taille();
	m_sommets.pousse(sommet);
}

void Maillage::ajoute_sommets(const dls::math::vec3f *sommets, size_t nombre)
{
	m_sommets.reserve(m_sommets.taille() + nombre);

	for (size_t i = 0; i < nombre; ++i) {
		ajoute_sommet(sommets[i]);
	}
}

void Maillage::reinitialise()
{
	for (auto &attr : m_attributs) {
		delete attr;
	}

	m_attributs.clear();
	m_sommets.reinitialise();
	m_polys.reinitialise();
	m_arretes.reinitialise();
	m_tableau_arretes.clear();
}

void Maillage::reserve_sommets(size_t nombre)
{
	m_sommets.reserve(nombre);
}

void Maillage::reserve_polygones(size_t nombre)
{
	m_polys.reserve(nombre);
}

ListePoints *Maillage::liste_points()
{
	return &m_sommets;
}

ListePolygones *Maillage::liste_polys()
{
	return &m_polys;
}

ListeSegments *Maillage::liste_segments()
{
	return &m_arretes;
}

Polygone *Maillage::ajoute_quad(const int s0, const int s1, const int s2, const int s3)
{
	auto poly = new Polygone();
	poly->s[0] = this->sommet(s0);
	poly->s[1] = this->sommet(s1);
	poly->s[2] = this->sommet(s2);
	poly->s[3] = ((s3 == -1) ? nullptr : this->sommet(s3));

	const auto nombre_sommet = ((s3 == -1) ? 3 : 4);

	for (int i = 0; i < nombre_sommet; ++i) {
		auto arrete = new Arrete();
		arrete->p = poly;

		arrete->s[0] = poly->s[i];
		arrete->s[1] = poly->s[(i + 1) % nombre_sommet];

		arrete->index = i;

		auto iter = m_tableau_arretes.find(std::make_pair(arrete->s[1]->index, arrete->s[0]->index));

		if (iter != m_tableau_arretes.end()) {
			arrete->opposee = iter->second;
			arrete->opposee->opposee = arrete;
		}
		else {
			arrete->opposee = nullptr;
		}

		m_tableau_arretes.insert({std::make_pair(arrete->s[0]->index, arrete->s[1]->index), arrete});

		poly->a[i] = arrete;

		m_arretes.pousse(arrete);
	}

	auto c1 = poly->s[1]->pos - poly->s[0]->pos;
	auto c2 = poly->s[2]->pos - poly->s[0]->pos;
	poly->nor = dls::math::normalise(dls::math::produit_croix(c1, c2));

	poly->index = m_polys.taille();

	m_polys.pousse(poly);

	return poly;
}

size_t Maillage::nombre_sommets() const
{
	return m_sommets.taille();
}

size_t Maillage::nombre_polygones() const
{
	return m_polys.taille();
}

Sommet *Maillage::sommet(size_t i)
{
	return *(m_sommets.sommets().begin() + i);
}

const Sommet *Maillage::sommet(size_t i) const
{
	return *(m_sommets.sommets().begin() + i);
}

const Polygone *Maillage::polygone(size_t i) const
{
	return *(m_polys.polys().begin() + i);
}

size_t Maillage::nombre_arretes() const
{
	return m_arretes.taille();
}

Arrete *Maillage::arrete(size_t i)
{
	return *(m_arretes.arretes().begin() + i);
}

const Arrete *Maillage::arrete(size_t i) const
{
	return *(m_arretes.arretes().begin() + i);
}

Polygone *Maillage::polygone(size_t i)
{
	return *(m_polys.polys().begin() + i);
}

static void min_max_vecteur(dls::math::point3f &min, dls::math::point3f &max, const dls::math::vec3f &v)
{
	for (int i = 0; i < 3; ++i) {
		if (v[i] < min[i]) {
			min[i] = v[i];
		}
		if (v[i] > max[i]) {
			max[i] = v[i];
		}
	}
}

void Maillage::calcule_boite_englobante()
{
	this->min = dls::math::point3f(std::numeric_limits<float>::max());
	this->max = dls::math::point3f(std::numeric_limits<float>::min());

	for (const auto &s : m_sommets.sommets()) {
		min_max_vecteur(this->min, this->max, s->pos);
	}

	this->taille = dls::math::point3f(this->max - this->min);
}

void Maillage::texture(TextureImage *t)
{
	m_texture = t;
}

TextureImage *Maillage::texture() const
{
	return m_texture;
}
