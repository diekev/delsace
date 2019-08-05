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

#include "maillage.hh"

#include "biblinternes/objets/creation.h"

#include "nuanceur.hh"
#include "types.hh"

namespace kdo {

Maillage::Maillage()
	: m_transformation(dls::math::mat4x4d(1.0))
	, m_nuanceur(nullptr)
{}

Maillage::~Maillage()
{
	for (auto &triangle : m_triangles) {
		memoire::deloge("kdo::Triangle", triangle);
	}

	m_triangles.efface();
	delete m_nuanceur;
}

Maillage::iterateur Maillage::begin()
{
	return m_triangles.debut();
}

Maillage::iterateur Maillage::end()
{
	return m_triangles.fin();
}

Maillage::const_iterateur Maillage::begin() const
{
	return m_triangles.debut();
}

Maillage::const_iterateur Maillage::end() const
{
	return m_triangles.fin();
}

void Maillage::ajoute_triangle(
		dls::math::vec3d const &v0,
		dls::math::vec3d const &v1,
		dls::math::vec3d const &v2)
{
	auto triangle = new Triangle;
	triangle->v0 = v0;
	triangle->v1 = v1;
	triangle->v2 = v2;
	triangle->normal = calcul_normal(*triangle);

	m_triangles.pousse(triangle);
}

void Maillage::transformation(math::transformation const &transforme)
{
	m_transformation = transforme;

	m_transformation(m_boite_englobante.min, &m_boite_englobante.min);
	m_transformation(m_boite_englobante.max, &m_boite_englobante.max);
}

math::transformation const &Maillage::transformation() const
{
	return m_transformation;
}

BoiteEnglobante const &Maillage::boite_englobante() const
{
	return m_boite_englobante;
}

void Maillage::nuanceur(Nuanceur *n)
{
	m_nuanceur = n;
}

Nuanceur *Maillage::nuanceur() const
{
	return m_nuanceur;
}

void Maillage::calcule_boite_englobante()
{
	dls::math::vec3d min(constantes<double>::INFINITE);
	dls::math::vec3d max(-constantes<double>::INFINITE);

	for (const Triangle *triangle : *this) {
		dls::math::extrait_min_max(triangle->v0, min, max);
		dls::math::extrait_min_max(triangle->v1, min, max);
		dls::math::extrait_min_max(triangle->v2, min, max);
	}

	m_boite_englobante = BoiteEnglobante(
							 dls::math::point3d(min),
							 dls::math::point3d(max));
}

void Maillage::calcule_limites(
		dls::math::vec3d const &normal,
		double &d_proche,
		double &d_eloigne) const
{
	for (auto const &tri : m_triangles) {
		auto d = dls::math::produit_scalaire(normal, tri->v0);

		if (d < d_proche) {
			d_proche = d;
		}
		if (d > d_eloigne) {
			d_eloigne = d;
		}

		d = dls::math::produit_scalaire(normal, tri->v1);

		if (d < d_proche) {
			d_proche = d;
		}
		if (d > d_eloigne) {
			d_eloigne = d;
		}

		d = dls::math::produit_scalaire(normal, tri->v2);

		if (d < d_proche) {
			d_proche = d;
		}
		if (d > d_eloigne) {
			d_eloigne = d;
		}
	}
}

void Maillage::nom(dls::chaine const &nom)
{
	m_nom = nom;
}

dls::chaine const &Maillage::nom() const
{
	return m_nom;
}

dls::math::vec3d calcul_normal(Triangle const &triangle)
{
	auto c1 = triangle.v1 - triangle.v0;
	auto c2 = triangle.v2 - triangle.v0;

	return dls::math::normalise(dls::math::produit_croix(c1, c2));
}

}  /* namespace kdo */
