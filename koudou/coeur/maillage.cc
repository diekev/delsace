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

#include "bibliotheques/objets/creation.h"

#include "commandes/adaptrice_creation_maillage.h"

#include "nuanceur.h"
#include "types.h"

Maillage::Maillage()
	: m_transformation(dls::math::mat4x4d(1.0))
	, m_nuanceur(nullptr)
{}

Maillage::~Maillage()
{
	for (auto &triangle : m_triangles) {
		delete triangle;
	}

	delete m_nuanceur;
}

Maillage::iterateur Maillage::begin()
{
	return m_triangles.begin();
}

Maillage::iterateur Maillage::end()
{
	return m_triangles.end();
}

Maillage::const_iterateur Maillage::begin() const
{
	return m_triangles.cbegin();
}

Maillage::const_iterateur Maillage::end() const
{
	return m_triangles.cend();
}

void Maillage::ajoute_triangle(
		const dls::math::vec3d &v0,
		const dls::math::vec3d &v1,
		const dls::math::vec3d &v2)
{
	auto triangle = new Triangle;
	triangle->v0 = v0;
	triangle->v1 = v1;
	triangle->v2 = v2;
	triangle->normal = calcul_normal(*triangle);

	m_triangles.push_back(triangle);
}

void Maillage::transformation(const math::transformation &transforme)
{
	m_transformation = transforme;

	m_transformation(m_boite_englobante.min, &m_boite_englobante.min);
	m_transformation(m_boite_englobante.max, &m_boite_englobante.max);
}

const math::transformation &Maillage::transformation() const
{
	return m_transformation;
}

const BoiteEnglobante &Maillage::boite_englobante() const
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

static void min_max_vecteur(
		const dls::math::vec3d &vecteur,
		dls::math::point3d &min,
		dls::math::point3d &max)
{
	for (size_t i = 0; i < 3; ++i) {
		min[i] = std::min(min[i], vecteur[i]);
		max[i] = std::max(max[i], vecteur[i]);
	}
}

void Maillage::calcule_boite_englobante()
{
	dls::math::point3d min(INFINITE);
	dls::math::point3d max(-INFINITE);

	for (const Triangle *triangle : *this) {
		min_max_vecteur(triangle->v0, min, max);
		min_max_vecteur(triangle->v1, min, max);
		min_max_vecteur(triangle->v2, min, max);
	}

	m_boite_englobante = BoiteEnglobante(min, max);
}

Maillage *Maillage::cree_cube()
{
	auto maillage = new Maillage;

	AdaptriceChargementMaillage adaptrice;
	adaptrice.maillage = maillage;

	objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);

	maillage->calcule_boite_englobante();
	maillage->nom("Cube");

	return maillage;
}

Maillage *Maillage::cree_sphere_uv()
{
	auto maillage = new Maillage;

	AdaptriceChargementMaillage adaptrice;
	adaptrice.maillage = maillage;

	objets::cree_sphere_uv(&adaptrice, 1.0f);

	maillage->calcule_boite_englobante();
	maillage->nom("Sphère");

	return maillage;
}

Maillage *Maillage::cree_plan()
{
	auto maillage = new Maillage;

	const dls::math::vec3d vertices[8] = {
		dls::math::vec3d(-5.0, 0.0, -5.0),
		dls::math::vec3d(-5.0, 0.0,  5.0),
		dls::math::vec3d( 5.0, 0.0,  5.0),
		dls::math::vec3d( 5.0, 0.0, -5.0),
	};

	maillage->ajoute_triangle(
				vertices[0],
				vertices[1],
				vertices[2]);

	maillage->ajoute_triangle(
				vertices[0],
				vertices[2],
				vertices[3]);

	maillage->calcule_boite_englobante();
	maillage->nom("Plan");

	return maillage;
}

void Maillage::calcule_limites(
		const dls::math::vec3d &normal,
		double &d_proche,
		double &d_eloigne) const
{
	for (const auto &tri : m_triangles) {
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

void Maillage::nom(const std::string &nom)
{
	m_nom = nom;
}

const std::string &Maillage::nom() const
{
	return m_nom;
}

dls::math::vec3d calcul_normal(const Triangle &triangle)
{
	auto c1 = triangle.v1 - triangle.v0;
	auto c2 = triangle.v2 - triangle.v0;

	return dls::math::normalise(dls::math::produit_croix(c1, c2));
}

/**
 * Algorithme de Möller-Trumbore.
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_entresection_algorithm
 */
bool entresecte_triangle(const Triangle &triangle, const Rayon &rayon, double &distance)
{
	constexpr auto epsilon = 0.000001;

	const auto &vertex0 = triangle.v0;
	const auto &vertex1 = triangle.v1;
	const auto &vertex2 = triangle.v2;

	const auto &cote1 = vertex1 - vertex0;
	const auto &cote2 = vertex2 - vertex0;
	const auto &h = dls::math::produit_croix(rayon.direction, cote2);
	const auto angle = dls::math::produit_scalaire(cote1, h);

	if (angle > -epsilon && angle < epsilon) {
		return false;
	}

	const auto f = 1 / angle;
	const auto &s = dls::math::vec3d(rayon.origine) - vertex0;
	const auto angle_u = f * dls::math::produit_scalaire(s, h);

	if (angle_u < 0.0 || angle_u > 1.0) {
		return false;
	}

	const auto q = dls::math::produit_croix(s, cote1);
	const auto angle_v = f * dls::math::produit_scalaire(rayon.direction, q);

	if (angle_v < 0.0 || angle_u + angle_v > 1.0) {
		return false;
	}

	/* À cette étape on peut calculer t pour trouver le point d'entresection sur
	 * la ligne. */
	const auto t = f * dls::math::produit_scalaire(cote2, q);

	/* Entresection avec le rayon. */
	if (t > epsilon) {
		distance = t;
		return true;
	}

	/* Cela veut dire qu'il y a une entresection avec une ligne, mais pas avec
	 * le rayon. */
	return false;
}
