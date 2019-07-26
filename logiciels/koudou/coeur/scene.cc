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

#include "scene.h"

#include <cassert>

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/texture/texture.h"

#include "koudou.h"
#include "lumiere.h"
#include "maillage.h"
#include "nuanceur.h"
#include "objet.h"
#include "statistiques.h"
#include "structure_acceleration.h"

/* ************************************************************************** */

Monde::~Monde()
{
	supprime_texture(texture);
}

Spectre spectre_monde(Monde const &monde, dls::math::vec3d const &direction)
{
	if (monde.texture) {
		return monde.texture->echantillone(direction);
	}

	float rvb[3] = { 1.0f, 0.0f, 1.0f };
	return Spectre::depuis_rgb(rvb);
}

/* ************************************************************************** */

Scene::Scene()
{
	/* Création du monde. */
	float rgb_monde[3] = {0.05f, 0.35f, 0.8f};

	Texture *texture = new TextureCouleur;

	auto texture_couleur = static_cast<TextureCouleur *>(texture);
	texture_couleur->etablie_spectre(Spectre::depuis_rgb(rgb_monde));

	monde.texture = texture;

	/* Création du soleil. */
	auto transformation = math::transformation();

	auto soleil = new LumiereDistante(transformation, Spectre(0.95));
	soleil->nuanceur = NuanceurEmission::defaut();

	ajoute_lumiere(soleil);

	transformation *= math::translation(0.0, 2.0, 4.0);

	float rgb1[3] = {1.0f, 0.6f, 0.6f};
	auto lumiere_point = new LumierePoint(transformation, Spectre::depuis_rgb(rgb1), 100);
	ajoute_lumiere(lumiere_point);
	lumiere_point->nuanceur = NuanceurEmission::defaut();

	transformation = math::transformation();
	transformation *= math::translation(-1.0, 4.0, -1.0);

	float rgb2[3] = {0.6f, 0.6f, 1.0f};
	lumiere_point = new LumierePoint(transformation, Spectre::depuis_rgb(rgb2), 100);
	lumiere_point->nuanceur = NuanceurEmission::defaut();
	ajoute_lumiere(lumiere_point);

#if 1
	transformation = math::transformation();
	transformation *= math::translation(0.0, 1.0, 1.5);

	auto maillage = Maillage::cree_sphere_uv();
	maillage->transformation(transformation);
	auto nuanceur = static_cast<NuanceurDiffus *>(NuanceurDiffus::defaut());

	float rouge[3] = { 1.0f, 0.0, 0.0 };
	nuanceur->spectre = Spectre::depuis_rgb(rouge);
	maillage->nuanceur(nuanceur);

	ajoute_maillage(maillage);

	transformation = math::transformation();
	transformation *= math::translation(0.0, 0.5, -2.0);
	transformation *= math::echelle(0.5, 0.5, 0.5);

	maillage = Maillage::cree_sphere_uv();
	maillage->transformation(transformation);
	maillage->nuanceur(NuanceurDiffus::defaut());

	ajoute_maillage(maillage);

	maillage = Maillage::cree_plan();
	maillage->nuanceur(NuanceurDiffus::defaut());
	ajoute_maillage(maillage);

#else
	auto maillage = Maillage::cree_cube();
	maillage->nuanceur(NuanceurVolume::defaut());

	transformation = math::transformation();
	transformation *= math::translation(0.0, 1.0, 0.0);

	maillage->transformation(transformation);

	ajoute_maillage(maillage);

	maillage = Maillage::cree_plan();
	maillage->nuanceur(NuanceurDiffus::defaut());
	ajoute_maillage(maillage);
#endif
}

Scene::~Scene()
{
	for (auto &objet : objets) {
		delete objet;
	}
}

void Scene::ajoute_maillage(Maillage *maillage)
{
	auto objet = new Objet(maillage);
	objet->transformation = maillage->transformation();
	objet->nuanceur = maillage->nuanceur();

	objets.pousse(objet);
	objet_actif = objet;

	maillages.pousse(maillage);
}

void Scene::ajoute_lumiere(Lumiere *lumiere)
{
	auto objet = new Objet(lumiere);
	objet->transformation = lumiere->transformation;
	objet->nuanceur = lumiere->nuanceur;

	objets.pousse(objet);
	objet_actif = objet;

	lumieres.pousse(lumiere);
}

/* ************************************************************************** */

dls::math::vec3d normale_scene(Scene const &scene, dls::math::point3d const &position, dls::phys::esectd const &entresection)
{
	switch (entresection.type) {
		default:
		case ESECT_OBJET_TYPE_AUCUN:
		case ESECT_OBJET_TYPE_LUMIERE:
		{
			return dls::math::vec3d(0.0);
		}
		case ESECT_OBJET_TYPE_TRIANGLE:
		{
			auto maillage = scene.maillages[entresection.idx_objet];
			auto triangle = maillage->begin() + static_cast<long int>(entresection.idx);
			return (*triangle)->normal;
		}
	}
}

double ombre_scene(ParametresRendu const &parametres, Scene const &scene, dls::phys::rayond const &rayon, double distance_maximale)
{
	auto entresection = parametres.acceleratrice->entresecte(scene, rayon, distance_maximale);

	if (entresection.type == ESECT_OBJET_TYPE_AUCUN) {
		return 1.0;
	}

	return 0.0;
}

Spectre spectre_lumiere(ParametresRendu const &parametres, Scene const &scene, GNA &gna, dls::math::point3d const &pos, dls::math::vec3d const &nor)
{
	/* Biais pour les rayons d'ombrage. À FAIRE : mettre dans les paramètres. */
	auto const biais = 1e-4;
	auto spectre = Spectre(0.0);

	dls::phys::rayond rayon;
	/* Déplace l'origine un temps soit peu pour éviter que le rayon n'entresecte
	 * l'objet lui-même lors du lancement de rayon d'ombrage. */
	rayon.origine = pos + nor * biais;

	/* Échantillone lumières. */
	for (const Lumiere *lumiere : scene.lumieres) {
		auto lumiere_distante = dynamic_cast<const LumiereDistante *>(lumiere);

		if (lumiere_distante != nullptr) {
			auto const direction = lumiere_distante->dir;
			auto const direction_op = dls::math::vec3d(
										  -direction.x,
										  -direction.y,
										  -direction.z);

			auto const angle = dls::math::produit_scalaire(direction_op, nor);

			if (angle <= 0.0) {
				continue;
			}

			rayon.direction = direction_op;

			for (size_t i = 0; i < 3; ++i) {
				rayon.direction_inverse[i] = 1.0 / rayon.direction[i];
			}

			auto const ombre = ombre_scene(parametres, scene, rayon, 1000.0);

			if (ombre > 0.0) {
				spectre += lumiere->spectre * static_cast<float>(lumiere->intensite * angle * ombre);
			}
		}

		auto lumiere_point = dynamic_cast<const LumierePoint *>(lumiere);

		if (lumiere_point != nullptr) {
			auto const direction = pos - lumiere_point->pos;
			auto const dist2 = dls::math::longueur_carree(direction);
			auto dist = sqrt(dist2);

			auto const direction_op = dls::math::vec3d(
										  -direction.x / dist,
										  -direction.y / dist,
										  -direction.z / dist);

			auto const angle = dls::math::produit_scalaire(direction_op, nor);

			if (angle <= 0.0) {
				continue;
			}

			rayon.direction = direction_op;

			for (size_t i = 0; i < 3; ++i) {
				rayon.direction_inverse[i] = 1.0 / rayon.direction[i];
			}

			auto const ombre = ombre_scene(parametres, scene, rayon, dist2);

			if (ombre > 0.0) {
				/* La contribution d'une lumière point est proportionelle à
				 * l'inverse de sa distance. */
				spectre += ((lumiere->spectre * static_cast<float>(lumiere->intensite))
							* static_cast<float>(1.0 / (4.0 * constantes<double>::PI * dist)))
						   * static_cast<float>(angle * ombre);
			}
		}
	}

	/* Échantillone ciel. */
	auto const point = 1000.0 * cosine_direction(gna, nor);
	auto const posv = dls::math::vec3d(pos.x, pos.y, pos.z); /* XXX - PAS BEAU. */
	auto const direction = dls::math::normalise(point - posv);
	rayon.direction = direction;
	for (size_t i = 0; i < 3; ++i) {
		rayon.direction_inverse[i] = 1.0 / rayon.direction[i];
	}
	spectre += spectre_monde(scene.monde, direction) * static_cast<float>(ombre_scene(parametres, scene, rayon, 1000.0));

	return spectre;
}

dls::math::vec3d get_brdf_ray(GNA &gna, dls::math::vec3d const &nor, dls::math::vec3d const &rd)
{
	if (gna.uniforme(0.0, 1.0) < 0.793) {
		return cosine_direction(gna, nor);
	}

	auto const p = reflect(nor, rd);
	return dls::math::normalise(p + cosine_direction(gna, p) * 0.1);
}

dls::math::vec3d cosine_direction(GNA &gna, dls::math::vec3d const &nor)
{
	auto r = dls::math::vec2d(gna.uniforme(0.0, 1.0) * constantes<double>::TAU, gna.uniforme(0.0, 1.0) * constantes<double>::TAU);
	auto sin_r = sin(r[0]);
	auto dr = dls::math::vec3d(sin_r * sin(r[1]), sin_r * cos(r[1]), cos(r[0]));

	if (dls::math::produit_scalaire(dr, nor) < 0.0) {
		dls::math::vec3d(-dr[0], -dr[1], -dr[2]);
	}

	return dr;
}

dls::math::vec3d reflect(dls::math::vec3d const &nor, dls::math::vec3d const &dir)
{
	return dir - (2.0 * produit_scalaire(dir, nor) * nor);
}
