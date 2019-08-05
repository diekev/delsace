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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "moteur_rendu_koudou.hh"

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/objet.h"

#include "corps/iteration_corps.hh"

#include "koudou/koudou.hh"
#include "koudou/lumiere.hh"
#include "koudou/maillage.hh"
#include "koudou/moteur_rendu.hh"
#include "koudou/nuanceur.hh"
#include "koudou/structure_acceleration.hh"

MoteurRenduKoudou::MoteurRenduKoudou()
	: m_koudou(new kdo::Koudou())
{}

MoteurRenduKoudou::~MoteurRenduKoudou()
{
	delete m_koudou;
}

const char *MoteurRenduKoudou::id() const
{
	return "koudou";
}

void MoteurRenduKoudou::calcule_rendu(
		StatistiquesRendu &stats,
		float *tampon,
		int hauteur,
		int largeur,
		bool rendu_final)
{
	m_camera->ajourne();
	m_koudou->camera = this->m_camera;
	m_koudou->parametres_rendu.camera = m_koudou->camera;

	auto &scene_koudou = m_koudou->parametres_rendu.scene;

	scene_koudou.reinitialise();

	/* ********************************************************************** */

	auto transformation = math::transformation();
	transformation *= math::translation(0.0, 2.0, 4.0);

	float rgb1[3] = {1.0f, 0.6f, 0.6f};
	auto lumiere_point = new kdo::LumierePoint(transformation, Spectre::depuis_rgb(rgb1), 100);
	scene_koudou.ajoute_lumiere(lumiere_point);
	lumiere_point->nuanceur = kdo::NuanceurEmission::defaut();

	for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
		auto objet = m_delegue->objet(i);

		objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			if (objet->type != type_objet::CORPS) {
				return;
			}

			auto const &corps = static_cast<DonneesCorps const *>(donnees)->corps;

			auto maillage = memoire::loge<kdo::Maillage>("Maillage");
			maillage->nuanceur(kdo::NuanceurDiffus::defaut());

			pour_chaque_polygone(corps, [&](Corps const &, Polygone *poly)
			{
				auto attr_N = corps.attribut("N");

				for (auto j = 2; j < poly->nombre_sommets(); ++j) {
					auto const &v0 = corps.point_transforme(poly->index_point(0));
					auto const &v1 = corps.point_transforme(poly->index_point(j - 1));
					auto const &v2 = corps.point_transforme(poly->index_point(j));

					auto tri = memoire::loge<kdo::Triangle>("kdo::Triangle");
					tri->v0 = dls::math::converti_type<double>(v0);
					tri->v1 = dls::math::converti_type<double>(v1);
					tri->v2 = dls::math::converti_type<double>(v2);

					if (attr_N) {
						if (attr_N->portee == portee_attr::PRIMITIVE) {
							tri->n0 = dls::math::converti_type<double>(attr_N->vec3(static_cast<long>(poly->index)));
							tri->n1 = tri->n0;
							tri->n2 = tri->n0;
						}
						else {
							tri->n0 = dls::math::converti_type<double>(attr_N->vec3(poly->index_point(0)));
							tri->n1 = dls::math::converti_type<double>(attr_N->vec3(poly->index_point(j - 1)));
							tri->n2 = dls::math::converti_type<double>(attr_N->vec3(poly->index_point(j)));
						}
					}
					else {
						tri->n0 = calcul_normal(*tri);
						tri->n1 = tri->n0;
						tri->n2 = tri->n0;
					}

					maillage->m_triangles.pousse(tri);
				}
			});

			scene_koudou.ajoute_maillage(maillage);
		});
	}

	/* ********************************************************************** */

	auto moteur_rendu = m_koudou->moteur_rendu;
	auto nombre_echantillons = 4; //m_koudou->parametres_rendu.nombre_echantillons;

#ifdef STATISTIQUES
	init_statistiques();
#endif

	moteur_rendu->reinitialise();

	moteur_rendu->pointeur_pellicule()->redimensionne(
				dls::math::Hauteur(m_camera->hauteur()),
				dls::math::Largeur(m_camera->largeur()));

	auto acceleratrice = static_cast<kdo::AccelArbreHBE *>(nullptr);

	if (m_koudou->parametres_rendu.acceleratrice == nullptr) {
		acceleratrice = new kdo::AccelArbreHBE(scene_koudou);
		m_koudou->parametres_rendu.acceleratrice = acceleratrice;
	}
	else {
		acceleratrice = dynamic_cast<kdo::AccelArbreHBE *>(m_koudou->parametres_rendu.acceleratrice);
	}

	acceleratrice->construit();

	/* Génère carreaux. */
	auto const largeur_carreau = m_koudou->parametres_rendu.largeur_carreau;
	auto const hauteur_carreau = m_koudou->parametres_rendu.hauteur_carreau;
	auto const largeur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().nombre_colonnes());
	auto const hauteur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().nombre_lignes());
	auto const carreaux_x = static_cast<unsigned>(std::ceil(static_cast<float>(largeur_pellicule) / static_cast<float>(largeur_carreau)));
	auto const carreaux_y = static_cast<unsigned>(std::ceil(static_cast<float>(hauteur_pellicule) / static_cast<float>(hauteur_carreau)));

	dls::tableau<kdo::CarreauPellicule> carreaux;
	carreaux.reserve(carreaux_x * carreaux_y);

	for (unsigned int i = 0; i < carreaux_x; ++i) {
		for (unsigned int j = 0; j < carreaux_y; ++j) {
			kdo::CarreauPellicule carreau;
			carreau.x = i * largeur_carreau;
			carreau.y = j * hauteur_carreau;
			carreau.largeur = std::min(largeur_carreau, largeur_pellicule - carreau.x);
			carreau.hauteur = std::min(hauteur_carreau, hauteur_pellicule - carreau.y);

			carreaux.pousse(carreau);
		}
	}

//	auto temps_ecoule = 0.0;
//	auto temps_restant = 0.0;
//	auto temps_echantillon = 0.0;
	auto e = 0u;

	for (; e < nombre_echantillons; ++e) {
		//m_notaire->signale_progres_temps(e + 1, temps_echantillon, temps_ecoule, temps_restant);

		if (/*this->is_cancelled() ||*/ moteur_rendu->est_arrete()) {
			break;
		}

//		auto const debut_echantillon = dls::chrono::compte_seconde();

		moteur_rendu->echantillone_scene(m_koudou->parametres_rendu, carreaux, e);

//		temps_echantillon = debut_echantillon.temps();
//		temps_ecoule += temps_echantillon;
//		temps_restant = (temps_ecoule / (e + 1.0) * nombre_echantillons) - temps_ecoule;

		//m_notaire->signale_rendu_fini();
		//m_notaire->signale_progres_avance(((static_cast<float>(e) + 1.0f) / static_cast<float>(nombre_echantillons)) * 100.0f);
	}

	/* Il est possible que le temps restant ne soit pas égal à 0.0 quand
	 * l'échantillonage est terminé à cause des imprécisions dues à l'estimation
	 * de celui-ci ou encore si le rendu a été arrêté par l'utilisateur ; donc
	 * on le met à 0.0 au cas où. */
//	temps_restant = 0.0;

#ifdef STATISTIQUES
	imprime_statistiques(std::cout);
#endif

	//m_notaire->signale_progres_temps(e + 1, temps_echantillon, temps_ecoule, temps_restant);

	auto const &ptr_pellicule = moteur_rendu->pellicule();

	for (int y = 0; y < hauteur; ++y) {
		for (int x = 0; x < largeur; ++x) {
			*tampon++ = static_cast<float>(ptr_pellicule[y][x].x);
			*tampon++ = static_cast<float>(ptr_pellicule[y][x].y);
			*tampon++ = static_cast<float>(ptr_pellicule[y][x].z);
			*tampon++ = 1.0f;

//			std::swap(tampon[idx0 + 0], tampon[idx1 + 0]);
//			std::swap(tampon[idx0 + 1], tampon[idx1 + 1]);
//			std::swap(tampon[idx0 + 2], tampon[idx1 + 2]);
//			std::swap(tampon[idx0 + 3], tampon[idx1 + 3]);
		}
	}
}
