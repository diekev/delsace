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
#include "biblinternes/phys/collision.hh"

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
		dls::math::extrait_min_max(this->points[triangle->v0], min, max);
		dls::math::extrait_min_max(this->points[triangle->v1], min, max);
		dls::math::extrait_min_max(this->points[triangle->v2], min, max);
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
		auto d = dls::math::produit_scalaire(normal, this->points[tri->v0]);

		if (d < d_proche) {
			d_proche = d;
		}
		if (d > d_eloigne) {
			d_eloigne = d;
		}

		d = dls::math::produit_scalaire(normal, this->points[tri->v1]);

		if (d < d_proche) {
			d_proche = d;
		}
		if (d > d_eloigne) {
			d_eloigne = d;
		}

		d = dls::math::produit_scalaire(normal, this->points[tri->v2]);

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

delegue_maillage::delegue_maillage(const maillage &m)
	: ptr_maillage(m)
{}

long delegue_maillage::nombre_elements() const
{
	return ptr_maillage.nombre_triangles + ptr_maillage.nombre_quads;
}

void delegue_maillage::coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
{
	cos.efface();

	if (idx < ptr_maillage.nombre_triangles) {
		auto idx_tri = idx * 3;

		cos.pousse(ptr_maillage.points[ptr_maillage.triangles[idx_tri    ]]);
		cos.pousse(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 1]]);
		cos.pousse(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 2]]);
	}
	else {
		auto idx_quad = (idx - ptr_maillage.nombre_triangles) * 4;

		cos.pousse(ptr_maillage.points[ptr_maillage.quads[idx_quad    ]]);
		cos.pousse(ptr_maillage.points[ptr_maillage.quads[idx_quad + 1]]);
		cos.pousse(ptr_maillage.points[ptr_maillage.quads[idx_quad + 2]]);
		cos.pousse(ptr_maillage.points[ptr_maillage.quads[idx_quad + 3]]);
	}
}

dls::phys::esectd delegue_maillage::intersecte_element(long idx, const dls::phys::rayond &rayon) const
{
	auto distance = 1000.0;
	auto entresection = dls::phys::esectd();
	entresection.type = ESECT_OBJET_TYPE_AUCUN;

	auto u = 0.0;
	auto v = 0.0;

	if (idx < ptr_maillage.nombre_triangles) {
		auto idx_tri = idx * 3;

		auto v0 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.triangles[idx_tri    ]]);
		auto v1 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 1]]);
		auto v2 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 2]]);

		if (entresecte_triangle(v0, v1, v2, rayon, distance, &u, &v)) {
	#ifdef STATISTIQUES
			statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
	#endif
			if (distance > 0.0 && distance < 1000.0) {
				entresection.idx_objet = ptr_maillage.index;
				entresection.idx = idx;
				entresection.distance = distance;
				entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
				entresection.touche = true;

				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_triangles[idx_tri    ]]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_triangles[idx_tri + 1]]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_triangles[idx_tri + 2]]);

				auto w = 1.0 - u - v;
				auto N = w * n0 + u * n1 + v * n2;
				entresection.normal = N;
			}
		}
	}
	else {
		auto idx_quad = (idx - ptr_maillage.nombre_triangles) * 4;

		auto v0 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.quads[idx_quad    ]]);
		auto v1 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.quads[idx_quad + 1]]);
		auto v2 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.quads[idx_quad + 2]]);
		auto v3 = dls::math::converti_type_point<double>(ptr_maillage.points[ptr_maillage.quads[idx_quad + 3]]);

		if (entresecte_triangle(v0, v1, v2, rayon, distance, &u, &v)) {
#ifdef STATISTIQUES
			statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
			if (distance > 0.0 && distance < 1000.0) {
				entresection.idx_objet = ptr_maillage.index;
				entresection.idx = idx;
				entresection.distance = distance;
				entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
				entresection.touche = true;

				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad    ]]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad + 1]]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad + 2]]);

				auto w = 1.0 - u - v;
				auto N = w * n0 + u * n1 + v * n2;
				entresection.normal = N;
			}
		}
		else  if (entresecte_triangle(v0, v2, v3, rayon, distance, &u, &v)) {
#ifdef STATISTIQUES
			statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
			if (distance > 0.0 && distance < 1000.0) {
				entresection.idx_objet = ptr_maillage.index;
				entresection.idx = idx;
				entresection.distance = distance;
				entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
				entresection.touche = true;

				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad    ]]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad + 2]]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[ptr_maillage.normaux_quads[idx_quad + 3]]);

				auto w = 1.0 - u - v;
				auto N = w * n0 + u * n1 + v * n2;
				entresection.normal = N;
			}
		}
	}

	return entresection;
}

maillage::maillage()
	: delegue(*this)
{}

void maillage::construit_arbre_hbe()
{
	arbre_hbe = bli::cree_arbre_bvh(delegue);
}

dls::phys::esectd maillage::traverse_arbre(const dls::phys::rayond &rayon)
{
	return bli::traverse(arbre_hbe, delegue, rayon);
}

limites3d maillage::calcule_limites()
{
	auto min = dls::math::vec3f( constantes<float>::INFINITE);
	auto max = dls::math::vec3f(-constantes<float>::INFINITE);

	for (auto const &point : points) {
		extrait_min_max(point, min, max);
	}

	auto lims = limites3d();
	lims.min = dls::math::converti_type<double>(min);
	lims.max = dls::math::converti_type<double>(max);
	return lims;
}

}  /* namespace kdo */
