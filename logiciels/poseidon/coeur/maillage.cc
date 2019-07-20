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

/* ************************************************************************** */

Maillage::Maillage()
	: m_transformation(dls::math::mat4x4d(1.0))
	, m_nom("maillage")
{}

Maillage::~Maillage()
{
	for (auto &arrete : m_arretes) {
		delete arrete;
	}

	for (auto &sommet : m_sommets) {
		delete sommet;
	}

	for (auto &poly : m_polys) {
		delete poly;
	}
}

void Maillage::ajoute_sommet(dls::math::vec3f const &coord)
{
	auto sommet = new Sommet();
	sommet->pos = coord;
	sommet->index = m_sommets.taille();
	m_sommets.pousse(sommet);
}

void Maillage::ajoute_sommets(const dls::math::vec3f *sommets, long nombre)
{
	m_sommets.reserve(m_sommets.taille() + nombre);

	for (auto i = 0; i < nombre; ++i) {
		ajoute_sommet(sommets[i]);
	}
}

void Maillage::ajoute_quad(const int s0, const int s1, const int s2, const int s3)
{
	auto poly = new Polygone();
	poly->s[0] = m_sommets[s0];
	poly->s[1] = m_sommets[s1];
	poly->s[2] = m_sommets[s2];
	poly->s[3] = ((s3 == -1) ? nullptr : m_sommets[s3]);

	auto const nombre_sommet = ((s3 == -1) ? 3l : 4l);

	for (auto i = 0; i < nombre_sommet; ++i) {
		auto arrete = new Arrete();
		arrete->p = poly;

		arrete->s[0] = poly->s[i];
		arrete->s[1] = poly->s[(i + 1) % nombre_sommet];

		arrete->index = i;

		auto iter = m_tableau_arretes.trouve(std::make_pair(arrete->s[1]->index, arrete->s[0]->index));

		if (iter != m_tableau_arretes.fin()) {
			arrete->opposee = iter->second;
			arrete->opposee->opposee = arrete;
		}
		else {
			arrete->opposee = nullptr;
		}

		m_tableau_arretes.insere({std::make_pair(arrete->s[0]->index, arrete->s[1]->index), arrete});

		poly->a[i] = arrete;

		m_arretes.pousse(arrete);
	}

	auto c1 = poly->s[1]->pos - poly->s[0]->pos;
	auto c2 = poly->s[2]->pos - poly->s[0]->pos;
	poly->nor =  dls::math::normalise(dls::math::produit_croix(c1, c2));

	poly->index = m_polys.taille();

	m_polys.pousse(poly);
}

dls::chaine const &Maillage::nom() const
{
	return m_nom;
}

void Maillage::nom(dls::chaine const &nom)
{
	m_nom = nom;
}

void Maillage::transformation(math::transformation const &transforme)
{
	m_transformation = transforme;
}

math::transformation const &Maillage::transformation() const
{
	return m_transformation;
}

long Maillage::nombre_sommets() const
{
	return m_sommets.taille();
}

long Maillage::nombre_polygones() const
{
	return m_polys.taille();
}

const Sommet *Maillage::sommet(long i) const
{
	return m_sommets[i];
}

const Polygone *Maillage::polygone(long i) const
{
	return m_polys[i];
}

long Maillage::nombre_arretes() const
{
	return m_arretes.taille();
}

Arrete *Maillage::arrete(long i)
{
	return m_arretes[i];
}

const Arrete *Maillage::arrete(long i) const
{
	return m_arretes[i];
}

Polygone *Maillage::polygone(long i)
{
	return m_polys[i];
}

void Maillage::calcule_boite_englobante()
{
	m_min = dls::math::vec3f( std::numeric_limits<float>::max());
	m_max = dls::math::vec3f(-std::numeric_limits<float>::max());

	for (auto const &s : m_sommets) {
		dls::math::extrait_min_max(s->pos, m_min, m_max);
	}

	m_taille = m_max - m_min;
}

dls::math::vec3f const &Maillage::min() const
{
	return m_min;
}

dls::math::vec3f const &Maillage::max() const
{
	return m_max;
}

dls::math::vec3f const &Maillage::taille() const
{
	return m_taille;
}
