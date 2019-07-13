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

#include "structure_acceleration.h"

#include "biblinternes/math/boite_englobante.hh"
#include "biblinternes/outils/definitions.h"

#include "maillage.h"
#include "scene.h"
#include "statistiques.h"

/* ************************************************************************** */

static void entresecte_triangles_maillage(
		Maillage const &maillage,
		long index,
		double distance_maximale,
		Rayon const &rayon,
		Entresection &entresection)
{
	auto index_triangle = 0l;

	for (const Triangle *triangle : maillage) {
#ifdef STATISTIQUES
		statistiques.test_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
		auto distance = distance_maximale;

		if (entresecte_triangle(*triangle, rayon, distance)) {
#ifdef STATISTIQUES
			statistiques.nombre_entresections_triangles.fetch_add(1, std::memory_order_relaxed);
#endif
			if (distance > 0.0 && distance < entresection.distance) {
				entresection.id = index;
				entresection.id_triangle = index_triangle;
				entresection.distance = distance;
				entresection.type_objet = OBJET_TYPE_TRIANGLE;
				entresection.maillage = &maillage;
			}
		}

		++index_triangle;
	}
}

/* Algorithme issu de
 * https://tavianator.com/fast-branchless-raybounding-box-entresections-part-2-nans/
 */
static bool entresecte_boite(BoiteEnglobante const &boite, Rayon const &rayon)
{
	auto t1 = (boite.min[0] - rayon.origine[0]) * rayon.inverse_direction[0];
	auto t2 = (boite.max[0] - rayon.origine[0]) * rayon.inverse_direction[0];

	auto tmin = std::min(t1, t2);
	auto tmax = std::max(t1, t2);

	for (size_t i = 1; i < 3; ++i) {
		t1 = (boite.min[i] - rayon.origine[i]) * rayon.inverse_direction[i];
		t2 = (boite.max[i] - rayon.origine[i]) * rayon.inverse_direction[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	return tmax > std::max(tmin, 0.0);
}

Entresection StructureAcceleration::entresecte(
		Scene const &scene,
		Rayon const &rayon,
		double distance_maximale) const
{
	auto index = 0l;
	auto entresection = Entresection();
	entresection.distance = distance_maximale;

	Rayon rayon_local;
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

bool VolumeEnglobant::Etendue::entresecte(Rayon const &rayon,
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

Entresection VolumeEnglobant::entresecte(
		Scene const &scene,
		Rayon const &rayon,
		double distance_maximale) const
{
	double numerateur_precalcule[NOMBRE_NORMAUX_PLAN];
	double denominateur_precalcule[NOMBRE_NORMAUX_PLAN];

	Rayon rayon_local;
	rayon_local.distance_min = rayon.distance_min;
	rayon_local.distance_max = rayon.distance_max;
	rayon_local.temps = rayon.temps;

	auto index = 0l;
	auto entresection = Entresection();
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
