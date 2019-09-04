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
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/objet.h"
#include "corps/iteration_corps.hh"
#include "corps/volume.hh"

#include "koudou/koudou.hh"
#include "koudou/lumiere.hh"
#include "koudou/maillage.hh"
#include "koudou/moteur_rendu.hh"
#include "koudou/nuanceur.hh"
#include "koudou/structure_acceleration.hh"

#include "wolika/grille_eparse.hh"

#include "rendu_corps.h"

/* ************************************************************************** */

enum {
	QUAD_X_MIN = 0,
	QUAD_X_MAX = 1,
	QUAD_Y_MIN = 2,
	QUAD_Y_MAX = 3,
	QUAD_Z_MIN = 4,
	QUAD_Z_MAX = 5,
};

const int quads_indices[6][4] = {
	/* QUAD_X_MIN */
	{4, 0, 3, 7},
	/* QUAD_X_MAX */
	{1, 5, 6, 2},
	/* QUAD_Y_MIN */
	{4, 5, 1, 0},
	/* QUAD_Y_MAX */
	{3, 2, 6, 7},
	/* QUAD_Z_MIN */
	{0, 1, 2, 3},
	/* QUAD_Z_MAX */
	{5, 4, 7, 6},
};

const dls::math::vec3d quads_normals[6] = {
	/* QUAD_X_MIN */
	dls::math::vec3d(-1.0, 0.0, 0.0),
	/* QUAD_X_MAX */
	dls::math::vec3d(1.0, 0.0, 0.0),
	/* QUAD_Y_MIN */
	dls::math::vec3d(0.0, -1.0, 0.0),
	/* QUAD_Y_MAX */
	dls::math::vec3d(0.0, 1.0, 0.0),
	/* QUAD_Z_MIN */
	dls::math::vec3d(0.0, 0.0, -1.0),
	/* QUAD_Z_MAX */
	dls::math::vec3d(0.0, 0.0, 1.0),
};

static int ajoute_vertex(
		dls::tableau<dls::math::vec3i> &vertex,
		dls::dico_desordonne<size_t, int> &utilises,
		dls::math::vec3i const &res,
		dls::math::vec3i const &v)
{
	auto vert_key = static_cast<size_t>(v.x + v.y * (res.x + 1) + v.z * (res.x + 1) * (res.y + 1));
	auto it = utilises.trouve(vert_key);

	if (it != utilises.fin()) {
		return it->second;
	}

	auto vertex_offset = static_cast<int>(vertex.taille());
	utilises[vert_key] = vertex_offset;
	vertex.pousse(v);
	return vertex_offset;
}

static void ajoute_quad(
		int i,
		kdo::Maillage *maillage,
		dls::tableau<dls::math::vec3i> &vertex,
		dls::dico_desordonne<size_t, int> &utilises,
		dls::math::vec3i const &res,
		dls::math::vec3i coins[8])
{
	auto decalage_normal = static_cast<int>(maillage->normaux.taille());

	auto tri = memoire::loge<kdo::Triangle>("kdo::Triangle");

	auto v0 = ajoute_vertex(vertex, utilises, res, coins[quads_indices[i][0]]);
	auto v1 = ajoute_vertex(vertex, utilises, res, coins[quads_indices[i][1]]);
	auto v2 = ajoute_vertex(vertex, utilises, res, coins[quads_indices[i][2]]);
	auto v3 = ajoute_vertex(vertex, utilises, res, coins[quads_indices[i][3]]);

	tri->v0 = v0;
	tri->v1 = v1;
	tri->v2 = v2;

	tri->n0 = decalage_normal;
	tri->n1 = decalage_normal;
	tri->n2 = decalage_normal;

	maillage->m_triangles.pousse(tri);

	tri = memoire::loge<kdo::Triangle>("kdo::Triangle");

	tri->v0 = v0;
	tri->v1 = v2;
	tri->v2 = v3;

	tri->n0 = decalage_normal;
	tri->n1 = decalage_normal;
	tri->n2 = decalage_normal;

	maillage->m_triangles.pousse(tri);

	maillage->normaux.pousse(quads_normals[i]);
}

/* ************************************************************************** */

/* À FAIRE : meilleure moyen de sélectionner les volumes, peut-être via un
 * éditeur de nuanceur ou un graphe de rendu. */
static auto volume_prim(Corps const &corps)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() == type_primitive::VOLUME) {
			return dynamic_cast<Volume *>(prim);
		}
	}

	return static_cast<Volume *>(nullptr);
}

static void ajoute_volume(
		kdo::Scene &scene,
		kdo::Maillage *maillage,
		Corps const &corps)
{
	//maillage->nuanceur(kdo::NuanceurDiffus::defaut());
	maillage->nuanceur(kdo::NuanceurVolume::defaut());

	auto volume = volume_prim(corps);

	if (!volume) {
		return;
	}

	auto grille = volume->grille;

	if (grille->est_eparse()) {
		auto grille_eprs = dynamic_cast<wlk::grille_eparse<float> *>(grille);

		scene.volumes.pousse(grille_eprs);
		maillage->volume = static_cast<int>(scene.volumes.taille() - 1);

		/* ajoute un cube pour chaque tuile de la grille */
		auto const &topologie = grille_eprs->topologie();
		auto res_tuile = grille_eprs->res_tuile();
		auto dalle_x = 1;
		auto dalle_y = res_tuile.x;
		auto dalle_z = res_tuile.x * res_tuile.y;

		/* Les points sont générés en espace index pour pouvoir mieux les
		 * dédupliquer. */
		auto vertex = dls::tableau<dls::math::vec3i>();
		auto utilises = dls::dico_desordonne<size_t, int>();

		auto index = 0;
		for (auto z = 0; z < res_tuile.z; ++z) {
			for (auto y = 0; y < res_tuile.y; ++y) {
				for (auto x = 0; x < res_tuile.x; ++x, ++index) {
					if (topologie[index] == -1) {
						continue;
					}

					auto min = dls::math::vec3i(x * 8, y * 8, z * 8);
					auto max = min + dls::math::vec3i(8);

					dls::math::vec3i coins[8] = {
						dls::math::vec3i(min[0], min[1], min[2]),
						dls::math::vec3i(max[0], min[1], min[2]),
						dls::math::vec3i(max[0], max[1], min[2]),
						dls::math::vec3i(min[0], max[1], min[2]),
						dls::math::vec3i(min[0], min[1], max[2]),
						dls::math::vec3i(max[0], min[1], max[2]),
						dls::math::vec3i(max[0], max[1], max[2]),
						dls::math::vec3i(min[0], max[1], max[2]),
					};

					if (x == 0 || topologie[index - dalle_x] == -1) {
						ajoute_quad(QUAD_X_MIN, maillage, vertex, utilises, res_tuile, coins);
					}

					if (y == 0 || topologie[index - dalle_y] == -1) {
						ajoute_quad(QUAD_Y_MIN, maillage, vertex, utilises, res_tuile, coins);
					}

					if (z == 0 || topologie[index - dalle_z] == -1) {
						ajoute_quad(QUAD_Z_MIN, maillage, vertex, utilises, res_tuile, coins);
					}

					if (x == (res_tuile.x - 1) || topologie[index + dalle_x] == -1) {
						ajoute_quad(QUAD_X_MAX, maillage, vertex, utilises, res_tuile, coins);
					}

					if (y == (res_tuile.y - 1) || topologie[index + dalle_y] == -1) {
						ajoute_quad(QUAD_Y_MAX, maillage, vertex, utilises, res_tuile, coins);
					}

					if (z == (res_tuile.z - 1) || topologie[index + dalle_z] == -1) {
						ajoute_quad(QUAD_Z_MAX, maillage, vertex, utilises, res_tuile, coins);
					}
				}
			}
		}

		for (auto const &v : vertex) {
			auto vmnd = grille_eprs->index_vers_monde(v);
			maillage->points.pousse(dls::math::converti_type<double>(vmnd));
		}
	}
}

static void ajoute_maillage(kdo::Maillage *maillage, Corps const &corps)
{
	maillage->nuanceur(kdo::NuanceurDiffus::defaut());

	auto points = corps.points_pour_lecture();
	maillage->points.reserve(points->taille());

	for (auto j = 0; j < points->taille(); ++j) {
		auto p = dls::math::converti_type<double>(corps.point_transforme(j));
		maillage->points.pousse(p);
	}

	auto attr_N = corps.attribut("N");

	if (attr_N) {
		maillage->normaux.reserve(attr_N->taille());

		for (auto j = 0; j < attr_N->taille(); ++j) {
			auto n = dls::math::vec3f();
			extrait(attr_N->r32(j), n);
			auto p = dls::math::converti_type<double>(n);
			maillage->normaux.pousse(p);
		}
	}

	pour_chaque_polygone(corps, [&](Corps const &, Polygone *poly)
	{
		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto tri = memoire::loge<kdo::Triangle>("kdo::Triangle");
			tri->v0 = static_cast<int>(poly->index_point(0));
			tri->v1 = static_cast<int>(poly->index_point(j - 1));
			tri->v2 = static_cast<int>(poly->index_point(j));

			if (attr_N) {
				if (attr_N->portee == portee_attr::PRIMITIVE) {
					tri->n0 = static_cast<int>(poly->index);
					tri->n1 = tri->n0;
					tri->n2 = tri->n0;
				}
				else {
					tri->n0 = static_cast<int>(poly->index_point(0));
					tri->n1 = static_cast<int>(poly->index_point(j - 1));
					tri->n2 = static_cast<int>(poly->index_point(j));
				}
			}
			else {
				auto const &v0 = maillage->points[tri->v0];
				auto const &v1 = maillage->points[tri->v1];
				auto const &v2 = maillage->points[tri->v2];

				auto e0 = v1 - v0;
				auto e1 = v2 - v0;

				auto N = normalise(produit_croix(e0, e1));

				tri->n0 = static_cast<int>(maillage->normaux.taille());
				tri->n1 = tri->n0;
				tri->n2 = tri->n0;

				maillage->normaux.pousse(N);
			}

			maillage->m_triangles.pousse(tri);
		}
	});
}

/* ************************************************************************** */

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

	stats.nombre_objets = 0;
	stats.nombre_points = 0;
	stats.nombre_polygones = 0;

	for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
		auto objet = m_delegue->objet(i);

		objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			if (objet->type == type_objet::CORPS) {
				auto const &corps = extrait_corps(donnees);

				auto maillage = memoire::loge<kdo::Maillage>("Maillage");

				if (possede_volume(corps)) {
					ajoute_volume(scene_koudou, maillage, corps);
				}
				else {
					ajoute_maillage(maillage, corps);
				}

				stats.nombre_points += maillage->points.taille();
				stats.nombre_polygones += maillage->m_triangles.taille();

				scene_koudou.ajoute_maillage(maillage);
			}
			else if (objet->type == type_objet::LUMIERE) {
				auto const &lumiere = extrait_lumiere(donnees);

				auto lumiere_koudou = static_cast<kdo::Lumiere *>(nullptr);

				if (lumiere.type == LUMIERE_POINT) {
					auto intensite = static_cast<double>(lumiere.intensite);
					auto spectre = lumiere.spectre;

					lumiere_koudou = memoire::loge<kdo::LumierePoint>(
								"kdo::LumierePoint",
								objet->transformation,
								Spectre::depuis_rgb(&spectre.r),
								intensite);
				}
				else if (lumiere.type == LUMIERE_DISTANTE) {
					auto intensite = static_cast<double>(lumiere.intensite);
					auto spectre = lumiere.spectre;

					lumiere_koudou = memoire::loge<kdo::LumiereDistante>(
								"kdo::LumiereDistante",
								objet->transformation,
								Spectre::depuis_rgb(&spectre.r),
								intensite);
				}

				scene_koudou.ajoute_lumiere(lumiere_koudou);
			}

			stats.nombre_objets += 1;
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
	auto const largeur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().largeur());
	auto const hauteur_pellicule = static_cast<unsigned>(moteur_rendu->pellicule().hauteur());
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

	auto temps_ecoule = 0.0;
//	auto temps_restant = 0.0;
	auto temps_echantillon = 0.0;
	auto e = 0u;

	for (; e < nombre_echantillons; ++e) {
		//m_notaire->signale_progres_temps(e + 1, temps_echantillon, temps_ecoule, temps_restant);

		if (/*this->is_cancelled() ||*/ moteur_rendu->est_arrete()) {
			break;
		}

		auto const debut_echantillon = dls::chrono::compte_seconde();

		moteur_rendu->echantillone_scene(m_koudou->parametres_rendu, carreaux, e);

		temps_echantillon = debut_echantillon.temps();
		temps_ecoule += temps_echantillon;
//		temps_restant = (temps_ecoule / (e + 1.0) * nombre_echantillons) - temps_ecoule;

		//m_notaire->signale_rendu_fini();
		//m_notaire->signale_progres_avance(((static_cast<float>(e) + 1.0f) / static_cast<float>(nombre_echantillons)) * 100.0f);
	}

	stats.temps = temps_ecoule;

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
	auto const &donnees = ptr_pellicule.donnees();

	auto index = 0;
	for (int y = 0; y < hauteur; ++y) {
		for (int x = 0; x < largeur; ++x, ++index) {
			auto v = dls::math::converti_type<float>(donnees.valeur(index));
			*tampon++ = v.x;
			*tampon++ = v.y;
			*tampon++ = v.z;
			*tampon++ = 1.0f;

//			std::swap(tampon[idx0 + 0], tampon[idx1 + 0]);
//			std::swap(tampon[idx0 + 1], tampon[idx1 + 1]);
//			std::swap(tampon[idx0 + 2], tampon[idx1 + 2]);
//			std::swap(tampon[idx0 + 3], tampon[idx1 + 3]);
		}
	}
}
