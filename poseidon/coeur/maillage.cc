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

void Maillage::ajoute_sommet(const dls::math::vec3f &coord)
{
	auto sommet = new Sommet();
	sommet->pos = coord;
	sommet->index = m_sommets.size();
	m_sommets.push_back(sommet);
}

void Maillage::ajoute_sommets(const dls::math::vec3f *sommets, size_t nombre)
{
	m_sommets.reserve(m_sommets.size() + nombre);

	for (size_t i = 0; i < nombre; ++i) {
		ajoute_sommet(sommets[i]);
	}
}

void Maillage::ajoute_quad(const int s0, const int s1, const int s2, const int s3)
{
	auto poly = new Polygone();
	poly->s[0] = m_sommets[static_cast<size_t>(s0)];
	poly->s[1] = m_sommets[static_cast<size_t>(s1)];
	poly->s[2] = m_sommets[static_cast<size_t>(s2)];
	poly->s[3] = ((s3 == -1) ? nullptr : m_sommets[static_cast<size_t>(s3)]);

	const auto nombre_sommet = ((s3 == -1) ? 3ul : 4ul);

	for (size_t i = 0; i < nombre_sommet; ++i) {
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

		m_arretes.push_back(arrete);
	}

	auto c1 = poly->s[1]->pos - poly->s[0]->pos;
	auto c2 = poly->s[2]->pos - poly->s[0]->pos;
	poly->nor =  dls::math::normalise(dls::math::produit_croix(c1, c2));

	poly->index = m_polys.size();

	m_polys.push_back(poly);
}

const std::string &Maillage::nom() const
{
	return m_nom;
}

void Maillage::nom(const std::string &nom)
{
	m_nom = nom;
}

void Maillage::transformation(const math::transformation &transforme)
{
	m_transformation = transforme;
}

const math::transformation &Maillage::transformation() const
{
	return m_transformation;
}

size_t Maillage::nombre_sommets() const
{
	return m_sommets.size();
}

size_t Maillage::nombre_polygones() const
{
	return m_polys.size();
}

const Sommet *Maillage::sommet(size_t i) const
{
	return m_sommets[i];
}

const Polygone *Maillage::polygone(size_t i) const
{
	return m_polys[i];
}

size_t Maillage::nombre_arretes() const
{
	return m_arretes.size();
}

Arrete *Maillage::arrete(size_t i)
{
	return m_arretes[i];
}

const Arrete *Maillage::arrete(size_t i) const
{
	return m_arretes[i];
}

Polygone *Maillage::polygone(size_t i)
{
	return m_polys[i];
}

static void min_max_vecteur(dls::math::vec3f &min, dls::math::vec3f &max, const dls::math::vec3f &v)
{
	for (size_t i = 0; i < 3; ++i) {
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
	m_min = dls::math::vec3f(std::numeric_limits<float>::max());
	m_max = dls::math::vec3f(std::numeric_limits<float>::min());

	for (const auto &s : m_sommets) {
		min_max_vecteur(m_min, m_max, s->pos);
	}

	m_taille = m_max - m_min;
}

const dls::math::vec3f &Maillage::min() const
{
	return m_min;
}

const dls::math::vec3f &Maillage::max() const
{
	return m_max;
}

const dls::math::vec3f &Maillage::taille() const
{
	return m_taille;
}
