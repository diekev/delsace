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

#include "scene.hh"

#include <cassert>

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/texture/texture.h"

#include "koudou.hh"
#include "lumiere.hh"
#include "maillage.hh"
#include "nuanceur.hh"
#include "sphere.hh"
#include "statistiques.hh"

namespace kdo {

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
	: delegue(*this)
{
	/* Création du monde. */
	float rgb_monde[3] = {0.05f, 0.35f, 0.8f};

	Texture *texture = new TextureCouleur;

	auto texture_couleur = static_cast<TextureCouleur *>(texture);
	texture_couleur->etablie_spectre(Spectre::depuis_rgb(rgb_monde));

	monde.texture = texture;
}

Scene::~Scene()
{
	reinitialise();
}

void Scene::reinitialise()
{
	for (auto &n : noeuds) {
		switch (n->type) {
			case type_noeud::LUMIERE:
			{
				auto lumiere = dynamic_cast<Lumiere *>(n);

				if (lumiere->type_l == type_lumiere::POINT) {
					auto ptr = dynamic_cast<LumierePoint *>(lumiere);
					memoire::deloge("kdo::LumierePoint", ptr);
				}
				else if (lumiere->type_l == type_lumiere::DISTANTE) {
					auto ptr = dynamic_cast<LumiereDistante *>(lumiere);
					memoire::deloge("kdo::LumiereDistante", ptr);
				}

				break;
			}
			case type_noeud::MAILLAGE:
			{
				auto maillage = dynamic_cast<kdo::maillage *>(n);
				memoire::deloge("kdo::maillage", maillage);
				break;
			}
			case type_noeud::SPHERE:
			{
				auto sphere = dynamic_cast<kdo::sphere *>(n);
				memoire::deloge("kdo::sphere", sphere);
				break;
			}
		}
	}

	memoire::deloge("ArbreBVH", arbre_hbe);

	noeuds.efface();
	volumes.efface();
}

void Scene::construit_arbre_hbe()
{
	arbre_hbe = bli::cree_arbre_bvh(delegue);

	for (auto n : noeuds) {
		if (n->type == type_noeud::LUMIERE) {
			continue;
		}

		n->construit_arbre_hbe();
	}
}

dls::phys::esectd Scene::traverse(const dls::phys::rayond &r) const
{
	return bli::traverse(arbre_hbe, delegue, r);
}

/* ************************************************************************** */

double ombre_scene(ParametresRendu const &parametres, Scene const &scene, dls::phys::rayond const &rayon, double distance_maximale)
{
	auto entresection = scene.traverse(rayon);

	if (entresection.type == ESECT_OBJET_TYPE_AUCUN) {
		return 1.0;
	}

	return 0.0;
}

Spectre spectre_lumiere(ParametresRendu const &parametres, Scene const &scene, GNA &gna, dls::math::point3d const &pos, dls::math::vec3d const &nor)
{
	/* Biais pour les rayons d'ombrage. */
	auto const biais = parametres.biais_ombre;
	auto spectre = Spectre(0.0);

	dls::phys::rayond rayon;
	/* Déplace l'origine un temps soit peu pour éviter que le rayon n'entresecte
	 * l'objet lui-même lors du lancement de rayon d'ombrage. */
	rayon.origine = pos + nor * biais;

	/* Échantillone lumières. */
	for (auto const *n : scene.noeuds) {
		if (n->type != type_noeud::LUMIERE) {
			continue;
		}

		auto lumiere = dynamic_cast<Lumiere const *>(n);

		switch (lumiere->type_l) {
			case type_lumiere::POINT:
			{
				auto lumiere_point = dynamic_cast<const LumierePoint *>(lumiere);

				auto const direction = pos - lumiere_point->pos;
				auto const dist2 = dls::math::longueur_carree(direction);
				auto dist = sqrt(dist2);
				auto const direction_op = -direction / dist;

				auto const angle = dls::math::produit_scalaire(direction_op, nor);

				if (angle <= 0.0) {
					continue;
				}

				rayon.direction = direction_op;
				calcul_direction_inverse(rayon);

				auto const ombre = ombre_scene(parametres, scene, rayon, dist2);

				if (ombre > 0.0) {
					/* La contribution d'une lumière point est proportionelle à
					 * l'inverse de sa distance. */
					spectre += ((lumiere->spectre * static_cast<float>(lumiere->intensite))
								* static_cast<float>(1.0 / (4.0 * constantes<double>::PI * dist)))
							   * static_cast<float>(angle * ombre);
				}

				break;
			}
			case type_lumiere::DISTANTE:
			{
				auto lumiere_distante = dynamic_cast<const LumiereDistante *>(lumiere);
				auto const direction = lumiere_distante->dir;
				auto const direction_op = -direction;

				auto const angle = dls::math::produit_scalaire(direction_op, nor);

				if (angle <= 0.0) {
					continue;
				}

				rayon.direction = direction_op;
				calcul_direction_inverse(rayon);

				auto const ombre = ombre_scene(parametres, scene, rayon, 1000.0);

				if (ombre > 0.0) {
					spectre += lumiere->spectre * static_cast<float>(lumiere->intensite * angle * ombre);
				}

				break;
			}
		}
	}

	/* Échantillone ciel. */
	auto const point = 1000.0 * cosine_direction(gna, nor);
	auto const posv = dls::math::vec3d(pos.x, pos.y, pos.z); /* XXX - PAS BEAU. */
	auto const direction = dls::math::normalise(point - posv);
	rayon.direction = direction;
	calcul_direction_inverse(rayon);
	spectre += spectre_monde(scene.monde, direction) * static_cast<float>(ombre_scene(parametres, scene, rayon, 1000.0));

	return spectre;
}

dls::math::vec3d get_brdf_ray(GNA &gna, dls::math::vec3d const &nor, dls::math::vec3d const &rd)
{
	if (gna.uniforme(0.0, 1.0) < 0.793) {
		return cosine_direction(gna, nor);
	}

	auto const p = reflechi(rd, nor);
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

delegue_scene::delegue_scene(const Scene &scene)
	: ptr_scene(scene)
{}

long delegue_scene::nombre_elements() const
{
	return ptr_scene.noeuds.taille();
}

void delegue_scene::coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
{
	auto n = ptr_scene.noeuds[idx];

	auto lims = n->calcule_limites();

	cos.efface();
	cos.ajoute(dls::math::converti_type<float>(lims.min));
	cos.ajoute(dls::math::converti_type<float>(lims.max));
}

dls::phys::esectd delegue_scene::intersecte_element(long idx, const dls::phys::rayond &r) const
{
	auto n = ptr_scene.noeuds[idx];
	return n->traverse_arbre(r);
}

}  /* namespace kdo */
