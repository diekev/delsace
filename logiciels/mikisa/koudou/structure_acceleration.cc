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

#include "structure_acceleration.hh"

#include "biblinternes/math/boite_englobante.hh"
#include "biblinternes/phys/collision.hh"
#include "biblinternes/outils/definitions.h"

#include "maillage.hh"
#include "objet.hh"
#include "scene.hh"
#include "statistiques.hh"

namespace kdo {

/* ************************************************************************** */

static void entresecte_triangles_maillage(
		Maillage const &maillage,
		long index,
		double distance_maximale,
		dls::phys::rayond const &rayon,
		dls::phys::esectd &entresection)
{
	auto index_triangle = 0l;

	for (const Triangle *triangle : maillage) {
#ifdef STATISTIQUES
		statistiques.test_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
		auto distance = distance_maximale;

		auto v0 = dls::math::point3d(maillage.points[triangle->v0]);
		auto v1 = dls::math::point3d(maillage.points[triangle->v1]);
		auto v2 = dls::math::point3d(maillage.points[triangle->v2]);

		if (entresecte_triangle(v0, v1, v2, rayon, distance)) {
#ifdef STATISTIQUES
			statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
			if (distance > 0.0 && distance < entresection.distance) {
				entresection.idx_objet = index;
				entresection.idx = index_triangle;
				entresection.distance = distance;
				entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
			}
		}

		++index_triangle;
	}
}

static bool entresecte_boite(BoiteEnglobante const &boite, dls::phys::rayond const &rayon)
{
	return entresection_rapide_min_max(rayon, boite.min, boite.max) > -0.5;
}

dls::phys::esectd StructureAcceleration::entresecte(
		Scene const &scene,
		dls::phys::rayond const &rayon,
		double distance_maximale) const
{
	auto index = 0l;
	auto entresection = dls::phys::esectd();
	entresection.distance = distance_maximale;

	dls::phys::rayond rayon_local;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;
	rayon_local.temps = rayon.temps;

	for (Maillage *maillage : scene.maillages) {
#ifdef STATISTIQUES
			statistiques.test_entresections_boites.fetch_add(1, std::memory_order_relaxed);
#endif
		if (entresecte_boite(maillage->boite_englobante(), rayon)) {
			auto const &transforme_inverse = inverse(maillage->transformation());

			transforme_inverse(rayon.origine, &rayon_local.origine);
			transforme_inverse(rayon.direction, &rayon_local.direction);

			entresecte_triangles_maillage(*maillage, index, distance_maximale, rayon_local, entresection);
		}

		++index;
	}

	return entresection;
}

/* ************************************************************************** */

const dls::math::vec3d VolumeEnglobant::NORMAUX_PLAN[VolumeEnglobant::NOMBRE_NORMAUX_PLAN] = {
	dls::math::vec3d(1, 0, 0),
	dls::math::vec3d(0, 1, 0),
	dls::math::vec3d(0, 0, 1),
	dls::math::vec3d( std::sqrt(3) / 3.,  std::sqrt(3) / 3., std::sqrt(3) / 3.),
	dls::math::vec3d(-std::sqrt(3) / 3.,  std::sqrt(3) / 3., std::sqrt(3) / 3.),
	dls::math::vec3d(-std::sqrt(3) / 3., -std::sqrt(3) / 3., std::sqrt(3) / 3.),
	dls::math::vec3d( std::sqrt(3) / 3., -std::sqrt(3) / 3., std::sqrt(3) / 3.)
};

VolumeEnglobant::Etendue::Etendue()
{
	for (int i = 0; i < NOMBRE_NORMAUX_PLAN; ++i) {
		d[i][0] =  constantes<double>::INFINITE;
		d[i][1] = -constantes<double>::INFINITE;
	}
}

bool VolumeEnglobant::Etendue::entresecte(dls::phys::rayond const &rayon,
		double *numerateur_precalcule,
		double *denominateur_precalcule,
		double &d_proche,
		double &d_eloigne,
		uint8_t &index_plan) const
{
	INUTILISE(rayon);

	for (uint8_t i = 0; i < NOMBRE_NORMAUX_PLAN; ++i) {
		auto tn = (d[i][0] - numerateur_precalcule[i]) / denominateur_precalcule[i];
		auto tf = (d[i][1] - numerateur_precalcule[i]) / denominateur_precalcule[i];

		if (denominateur_precalcule[i] < 0) {
			std::swap(tn, tf);
		}

		if (tn > d_proche) {
			d_proche = tn;
			index_plan = i;
		}

		if (tf < d_eloigne) {
			d_eloigne = tf;
		}

		if (d_proche > d_eloigne) return false;
	}

	return true;
}


void VolumeEnglobant::construit(Scene const &scene)
{
	m_etendues.redimensionne(scene.maillages.taille());
	auto i = 0l;
	for (Maillage *maillage : scene.maillages) {
		for (auto j = 0; j < NOMBRE_NORMAUX_PLAN; ++j) {
			maillage->calcule_limites(NORMAUX_PLAN[j], m_etendues[i].d[j][0], m_etendues[i].d[j][1]);
		}

		++i;
	}
}

dls::phys::esectd VolumeEnglobant::entresecte(
		Scene const &scene,
		dls::phys::rayond const &rayon,
		double distance_maximale) const
{
	double numerateur_precalcule[NOMBRE_NORMAUX_PLAN];
	double denominateur_precalcule[NOMBRE_NORMAUX_PLAN];

	dls::phys::rayond rayon_local;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;
	rayon_local.temps = rayon.temps;

	auto index = 0l;
	auto entresection = dls::phys::esectd();
	entresection.distance = distance_maximale;

	for (Maillage *maillage : scene.maillages) {
#ifdef STATISTIQUES
		statistiques.test_entresections_volumes.fetch_add(1, std::memory_order_relaxed);
#endif

		auto d_proche = -constantes<double>::INFINITE;
		auto d_eloigne = constantes<double>::INFINITE;
		uint8_t index_plan;
		auto const &transforme_inverse = inverse(maillage->transformation());

		transforme_inverse(rayon.origine, &rayon_local.origine);
		transforme_inverse(rayon.direction, &rayon_local.direction);

		for (uint8_t i = 0; i < NOMBRE_NORMAUX_PLAN; ++i) {
			numerateur_precalcule[i] = dls::math::produit_scalaire(NORMAUX_PLAN[i], dls::math::vec3d(rayon_local.origine));
			denominateur_precalcule[i] = dls::math::produit_scalaire(NORMAUX_PLAN[i], rayon_local.direction);
		}

		bool touche = m_etendues[index].entresecte(
						  rayon,
						  numerateur_precalcule,
						  denominateur_precalcule,
						  d_proche,
						  d_eloigne,
						  index_plan);

		if (touche) {
			if (d_proche < distance_maximale) {

				entresecte_triangles_maillage(*maillage, index, distance_maximale, rayon_local, entresection);
				//entresection.normal = NORMAUX_PLAN[index_plan];
			}
		}

		++index;
	}

	return entresection;
}

/* ************************************************************************** */

DelegueScene::DelegueScene(const Scene &scene)
	: m_scene(scene)
{}

long DelegueScene::nombre_elements() const
{
	auto nombre_triangle = 0l;

	for (auto &maillage : m_scene.maillages) {
		nombre_triangle += maillage->m_triangles.taille();
	}

	return nombre_triangle;
}

BoiteEnglobante DelegueScene::boite_englobante(long idx) const
{
	auto nombre_triangle = 0l;
	auto maillage = static_cast<Maillage *>(nullptr);

	for (auto &maillage_ : m_scene.maillages) {
		if (nombre_triangle + maillage_->m_triangles.taille() > idx) {
			maillage = maillage_;
			break;
		}

		nombre_triangle += maillage_->m_triangles.taille();
	}

	auto triangle = maillage->m_triangles[idx - nombre_triangle];

	auto v0 = dls::math::point3d(maillage->points[triangle->v0]);
	auto v1 = dls::math::point3d(maillage->points[triangle->v1]);
	auto v2 = dls::math::point3d(maillage->points[triangle->v2]);

	auto boite = BoiteEnglobante();
	dls::math::extrait_min_max(v0, boite.min, boite.max);
	dls::math::extrait_min_max(v1, boite.min, boite.max);
	dls::math::extrait_min_max(v2, boite.min, boite.max);
	boite.id = idx;
	boite.centroide = (boite.min + boite.max) * 0.5;

	return boite;
}

dls::phys::esectd DelegueScene::intersecte_element(long idx, const dls::phys::rayond &r) const
{
	auto idx_objet = 0;
	auto nombre_triangle = 0l;
	auto maillage = static_cast<Maillage *>(nullptr);

	for (auto &maillage_ : m_scene.maillages) {
		if (nombre_triangle + maillage_->m_triangles.taille() > idx) {
			maillage = maillage_;
			break;
		}

		nombre_triangle += maillage_->m_triangles.taille();
		idx_objet += 1;
	}

	auto idx_triangle = idx - nombre_triangle;
	auto triangle = maillage->m_triangles[idx_triangle];

	auto distance = 1000.0;

	auto v0 = dls::math::point3d(maillage->points[triangle->v0]);
	auto v1 = dls::math::point3d(maillage->points[triangle->v1]);
	auto v2 = dls::math::point3d(maillage->points[triangle->v2]);

	auto entresection = dls::phys::esectd();
	entresection.type = ESECT_OBJET_TYPE_AUCUN;

	auto u = 0.0;
	auto v = 0.0;

	if (entresecte_triangle(v0, v1, v2, r, distance, &u, &v)) {
#ifdef STATISTIQUES
		statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
		if (distance > 0.0 && distance < 1000.0) {
			entresection.idx_objet = idx_objet;
			entresection.idx = idx_triangle;
			entresection.distance = distance;
			entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
			entresection.touche = true;

			auto n0 = maillage->normaux[triangle->n0];
			auto n1 = maillage->normaux[triangle->n1];
			auto n2 = maillage->normaux[triangle->n2];

			auto w = 1.0 - u - v;
			auto N = w * n0 + u * n1 + v * n2;
			entresection.normal = N;
		}
	}

	return entresection;
}

/* ************************************************************************** */

AccelArbreHBE::AccelArbreHBE(const Scene &scene)
	: m_delegue(scene)
{}

void AccelArbreHBE::construit()
{
	m_arbre = construit_arbre_hbe(m_delegue, 24);
}

dls::phys::esectd AccelArbreHBE::entresecte(const Scene &scene, const dls::phys::rayond &rayon, double distance_maximale) const
{
	auto accumulatrice = AccumulatriceTraverse(rayon.origine);
	traverse(m_arbre, m_delegue, rayon, accumulatrice);
	auto entresection = accumulatrice.intersection();

	if (entresection.touche) {
		entresection.type = ESECT_OBJET_TYPE_TRIANGLE;
	}
	else {
		entresection.type = ESECT_OBJET_TYPE_AUCUN;
	}

	return entresection;
}

/* ************************************************************************** */

}  /* namespace kdo */
