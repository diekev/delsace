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

		cos.ajoute(ptr_maillage.points[ptr_maillage.triangles[idx_tri    ]]);
		cos.ajoute(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 1]]);
		cos.ajoute(ptr_maillage.points[ptr_maillage.triangles[idx_tri + 2]]);
	}
	else {
		auto idx_quad = (idx - ptr_maillage.nombre_triangles) * 4;

		cos.ajoute(ptr_maillage.points[ptr_maillage.quads[idx_quad    ]]);
		cos.ajoute(ptr_maillage.points[ptr_maillage.quads[idx_quad + 1]]);
		cos.ajoute(ptr_maillage.points[ptr_maillage.quads[idx_quad + 2]]);
		cos.ajoute(ptr_maillage.points[ptr_maillage.quads[idx_quad + 3]]);
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

				auto in0 = ptr_maillage.normaux_triangles[idx_tri    ];
				auto in1 = ptr_maillage.normaux_triangles[idx_tri + 1];
				auto in2 = ptr_maillage.normaux_triangles[idx_tri + 2];

#ifdef COMPRESSE_NORMAUX
				auto n0q = dequantifie_vec3(ptr_maillage.normaux[in0]);
				auto n1q = dequantifie_vec3(ptr_maillage.normaux[in1]);
				auto n2q = dequantifie_vec3(ptr_maillage.normaux[in2]);

				auto n0 = dls::math::converti_type<double>(decode(n0q));
				auto n1 = dls::math::converti_type<double>(decode(n1q));
				auto n2 = dls::math::converti_type<double>(decode(n2q));
#else
				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[in0]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[in1]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[in2]);
#endif

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

				auto in0 = ptr_maillage.normaux_quads[idx_quad    ];
				auto in1 = ptr_maillage.normaux_quads[idx_quad + 1];
				auto in2 = ptr_maillage.normaux_quads[idx_quad + 2];

#ifdef COMPRESSE_NORMAUX
				auto n0q = dequantifie_vec3(ptr_maillage.normaux[in0]);
				auto n1q = dequantifie_vec3(ptr_maillage.normaux[in1]);
				auto n2q = dequantifie_vec3(ptr_maillage.normaux[in2]);

				auto n0 = dls::math::converti_type<double>(decode(n0q));
				auto n1 = dls::math::converti_type<double>(decode(n1q));
				auto n2 = dls::math::converti_type<double>(decode(n2q));
#else
				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[in0]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[in1]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[in2]);
#endif
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

				auto in0 = ptr_maillage.normaux_quads[idx_quad    ];
				auto in1 = ptr_maillage.normaux_quads[idx_quad + 2];
				auto in2 = ptr_maillage.normaux_quads[idx_quad + 3];

#ifdef COMPRESSE_NORMAUX
				auto n0q = dequantifie_vec3(ptr_maillage.normaux[in0]);
				auto n1q = dequantifie_vec3(ptr_maillage.normaux[in1]);
				auto n2q = dequantifie_vec3(ptr_maillage.normaux[in2]);

				auto n0 = dls::math::converti_type<double>(decode(n0q));
				auto n1 = dls::math::converti_type<double>(decode(n1q));
				auto n2 = dls::math::converti_type<double>(decode(n2q));
#else
				auto n0 = dls::math::converti_type<double>(ptr_maillage.normaux[in0]);
				auto n1 = dls::math::converti_type<double>(ptr_maillage.normaux[in1]);
				auto n2 = dls::math::converti_type<double>(ptr_maillage.normaux[in2]);
#endif
				auto w = 1.0 - u - v;
				auto N = w * n0 + u * n1 + v * n2;
				entresection.normal = N;
			}
		}
	}

	return entresection;
}

maillage::maillage()
	: noeud(type_noeud::MAILLAGE)
	, delegue(*this)
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
