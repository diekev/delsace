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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "moteur_rendu.h"

#include <tbb/parallel_for.h>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/vision/camera.h"

#include "bsdf.h"
#include "gna.h"
#include "koudou.h"
#include "maillage.h"
#include "nuanceur.h"
#include "statistiques.h"
#include "structure_acceleration.h"
#include "types.h"
#include "volume.h"

/* ************************************************************************** */

vision::EchantillonCamera genere_echantillon(GNA &gna, unsigned int x, unsigned int y)
{
	vision::EchantillonCamera echantillon;
	echantillon.x = x + gna.nombre_aleatoire();
	echantillon.y = y + gna.nombre_aleatoire();

	return echantillon;
}

static Rayon genere_rayon(vision::Camera3D *camera, vision::EchantillonCamera const &echantillon)
{
	auto pos = dls::math::point3f(
				   static_cast<float>(echantillon.x / camera->largeur()),
				   static_cast<float>((camera->hauteur() - echantillon.y) / camera->hauteur()),
				   0.0f);

	auto const &debut = camera->pos_monde(pos);

	pos.z = 1.0f;

	auto const &fin = camera->pos_monde(pos);

	auto const origine = camera->pos();
	auto const direction = normalise(fin - debut);

	Rayon r;
	r.origine = dls::math::point3d(origine.x, origine.y, origine.z);
	r.direction = dls::math::vec3d(direction.x, direction.y, direction.z);

	for (size_t i = 0; i < 3; ++i) {
		r.inverse_direction[i] = 1.0 / r.direction[i];
	}

	r.distance_min = 0.0;
	r.distance_max = constantes<double>::INFINITE;

	return r;
}

Spectre calcul_spectre(GNA &gna, ParametresRendu const &parametres, Rayon const &rayon, uint profondeur)
{
	Scene const &scene = parametres.scene;

	if (profondeur > 5) {
		auto point = rayon.origine + rayon.direction;
		auto vecteur = dls::math::vec3d(point);
		return spectre_monde(scene.monde, vecteur);
	}

	auto spectre_pixel = Spectre(0.0);
	auto spectre_entresection = Spectre(1.0);

	Rayon rayon_local = rayon;
	ContexteNuancage contexte;

	for (auto i = 0u; i < parametres.nombre_rebonds; ++i) {
		auto const entresection = parametres.acceleratrice->entresecte(
									  scene,
									  rayon_local,
									  1000.0);

		if (entresection.type_objet == OBJET_TYPE_AUCUN) {
			if (i == 0) {
				auto point = dls::math::vec3d(rayon_local.origine) + rayon_local.direction;
				auto vecteur = point;
				return spectre_monde(scene.monde, vecteur);
			}

			break;
		}

		// get pos and normal at the entresection point
		contexte.P = rayon_local.origine + entresection.distance * dls::math::point3d(rayon_local.direction);
		contexte.N = normale_scene(scene, contexte.P, entresection);
		contexte.V = -rayon_local.direction;

		contexte.rayon = rayon_local;

		// get color from the surface
		auto nuanceur = entresection.maillage->nuanceur();

		if (nuanceur->a_volume()) {
			auto volume = nuanceur->cree_volume(contexte);
			Spectre Lv;
			Spectre transmittance;
			Spectre poids;
			Rayon wo;

			if (!volume->integre(gna, parametres, Lv, transmittance, poids, wo)) {
				delete volume;
				break;
			}

			spectre_entresection *= transmittance;
			spectre_pixel += poids * spectre_entresection * Lv;

			delete volume;
		}
		else {
			Spectre Ls;
			double pdf;
			auto bsdf = nuanceur->cree_BSDF(contexte);
			bsdf->genere_echantillon(gna, parametres, rayon_local.direction, Ls, pdf, profondeur);

			spectre_entresection *= (Ls / static_cast<float>(pdf));
			spectre_pixel += spectre_entresection;

			delete bsdf;
		}

		rayon_local.origine = contexte.P;


		if (spectre_entresection.y() <= 0.1f) {
			break;
		}

		for (size_t j = 0; j < 3; ++j) {
			rayon_local.inverse_direction[j] = 1.0 / rayon_local.direction[j];
		}
	}

	return spectre_pixel;
}

/* ************************************************************************** */

void MoteurRendu::echantillone_scene(ParametresRendu const &parametres, dls::tableau<CarreauPellicule> const &carreaux, unsigned int echantillon)
{
	auto camera = parametres.camera;
	/* À FAIRE : redimensionne la caméra et la pellicule selon la fenêtre
	 * entreactive. */
//	auto vielle_hauteur = camera->hauteur();
//	auto vielle_largeur = camera->largeur();

	m_pellicule.redimensionne(dls::math::Hauteur(camera->hauteur()),
							  dls::math::Largeur(camera->largeur()));

	//camera->redimensionne(m_pellicule.largeur(), m_pellicule.hauteur());
	//camera->ajourne();

	tbb::parallel_for(tbb::blocked_range<long>(0, carreaux.taille()),
					  [&](tbb::blocked_range<long> const &plage)
	{
		GNA gna(17771 + static_cast<unsigned int>(plage.begin()) * (echantillon + 1));

		for (auto j = plage.begin(); j < plage.end(); ++j) {
			auto const &carreau = carreaux[j];

			for (auto x = carreau.x; x < carreau.x + carreau.largeur; ++x) {
				for (auto y = carreau.y; y < carreau.y + carreau.hauteur; ++y) {
					auto echantillon_camera = genere_echantillon(gna, x, y);

					Rayon rayon = genere_rayon(camera, echantillon_camera);
#ifdef STATISTIQUES
					statistiques.nombre_rayons_primaires.fetch_add(1, std::memory_order_relaxed);
#endif

					auto spectre_actuel = calcul_spectre(gna, parametres, rayon);

					/* Corrige gamma. */
					spectre_actuel = puissance(spectre_actuel, 0.45f);

					float rgb[3];
					spectre_actuel.vers_rvb(rgb);

					m_pellicule.ajoute_echantillon(x, y, dls::math::vec3d(rgb[0], rgb[1], rgb[2]));
				}
			}
		}
	});

//	camera->redimensionne(vielle_largeur, vielle_hauteur);
//	camera->ajourne();
}

void MoteurRendu::reinitialise()
{
	m_pellicule.reinitialise();
	m_est_arrete = false;
}

Pellicule *MoteurRendu::pointeur_pellicule()
{
	return &m_pellicule;
}

dls::math::matrice_dyn<dls::math::vec3d> const &MoteurRendu::pellicule()
{
	m_pellicule.creer_image();
	return m_pellicule.donnees();
}

void MoteurRendu::arrete()
{
	m_est_arrete = true;
}

bool MoteurRendu::est_arrete() const
{
	return m_est_arrete;
}

/* ************************************************************************** */

TacheRendu::TacheRendu(Koudou const &koudou)
	: Tache(koudou)
{}

void TacheRendu::commence(Koudou const &koudou)
{
	auto moteur_rendu = koudou.moteur_rendu;
	auto nombre_echantillons = koudou.parametres_rendu.nombre_echantillons;

#ifdef STATISTIQUES
	init_statistiques();
#endif

	moteur_rendu->reinitialise();

	auto volume_englobant = dynamic_cast<VolumeEnglobant *>(koudou.parametres_rendu.acceleratrice);

	if (volume_englobant != nullptr) {
		volume_englobant->construit(koudou.parametres_rendu.scene);
	}

	/* Génère carreaux. */
	auto const largeur_carreau = koudou.parametres_rendu.largeur_carreau;
	auto const hauteur_carreau = koudou.parametres_rendu.hauteur_carreau;
	auto const largeur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().nombre_colonnes());
	auto const hauteur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().nombre_lignes());
	auto const carreaux_x = static_cast<unsigned>(std::ceil(static_cast<float>(largeur_pellicule) / static_cast<float>(largeur_carreau)));
	auto const carreaux_y = static_cast<unsigned>(std::ceil(static_cast<float>(hauteur_pellicule) / static_cast<float>(hauteur_carreau)));

	dls::tableau<CarreauPellicule> carreaux;
	carreaux.reserve(static_cast<size_t>(carreaux_x * carreaux_y));

	for (unsigned int i = 0; i < carreaux_x; ++i) {
		for (unsigned int j = 0; j < carreaux_y; ++j) {
			CarreauPellicule carreau;
			carreau.x = i * largeur_carreau;
			carreau.y = j * hauteur_carreau;
			carreau.largeur = std::min(largeur_carreau, largeur_pellicule - carreau.x);
			carreau.hauteur = std::min(hauteur_carreau, hauteur_pellicule - carreau.y);

			carreaux.pousse(carreau);
		}
	}

	auto temps_ecoule = 0.0;
	auto temps_restant = 0.0;
	auto temps_echantillon = 0.0;
	auto e = 0u;

	for (; e < nombre_echantillons; ++e) {
		m_notaire->signale_progres_temps(e + 1, temps_echantillon, temps_ecoule, temps_restant);

		if (this->is_cancelled() || moteur_rendu->est_arrete()) {
			break;
		}

		auto const debut_echantillon = dls::chrono::maintenant();

		moteur_rendu->echantillone_scene(koudou.parametres_rendu, carreaux, e);

		auto const fin_echantillon = dls::chrono::maintenant();

		temps_echantillon = fin_echantillon - debut_echantillon;
		temps_ecoule += temps_echantillon;
		temps_restant = (temps_ecoule / (e + 1.0) * nombre_echantillons) - temps_ecoule;

		m_notaire->signale_rendu_fini();
		m_notaire->signale_progres_avance(((static_cast<float>(e) + 1.0f) / static_cast<float>(nombre_echantillons)) * 100.0f);
	}

	/* Il est possible que le temps restant ne soit pas égal à 0.0 quand
	 * l'échantillonage est terminé à cause des imprécisions dues à l'estimation
	 * de celui-ci ou encore si le rendu a été arrêté par l'utilisateur ; donc
	 * on le met à 0.0 au cas où. */
	temps_restant = 0.0;

#ifdef STATISTIQUES
	imprime_statistiques(std::cout);
#endif

	m_notaire->signale_progres_temps(e + 1, temps_echantillon, temps_ecoule, temps_restant);
}
