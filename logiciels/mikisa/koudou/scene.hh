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

#pragma once

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/rayon.hh"
#include "biblinternes/phys/spectre.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/arbre_hbe.hh"

#include "wolika/grille_eparse.hh"

#include "types.hh"

class GNA;
class Texture;

namespace kdo {

class Lumiere;
class Nuanceur;
struct ParametresRendu;

struct noeud;

/* ************************************************************************** */

struct Monde {
	Texture *texture = nullptr;

	Monde() = default;

	Monde(Monde const &) = default;
	Monde &operator=(Monde const &) = default;

	~Monde();
};

Spectre spectre_monde(Monde const &monde, dls::math::vec3d const &direction);

/* ************************************************************************** */

struct Scene;

struct delegue_scene {
	Scene const &ptr_scene;

	delegue_scene(Scene const &scene);

	long nombre_elements() const;

	void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const;

	dls::phys::esectd intersecte_element(long idx, const dls::phys::rayond &r) const;
};

struct Scene {
	Monde monde{};

	dls::tableau<wlk::grille_eparse<float> *> volumes{};

	dls::tableau<noeud *> noeuds{};
	delegue_scene delegue;
	bli::BVHTree *arbre_hbe = nullptr;

	Scene();
	~Scene();

	Scene(Scene const &) = default;
	Scene &operator=(Scene const &) = default;

	void reinitialise();

	void construit_arbre_hbe();

	dls::phys::esectd traverse(dls::phys::rayond const &r) const;
};

/* ************************************************************************** */

double ombre_scene(ParametresRendu const &parametres, Scene const &scene, dls::phys::rayond const &rayon, double distance_maximale);

Spectre spectre_lumiere(ParametresRendu const &parametres, Scene const &scene, GNA &gna, dls::math::point3d const &pos, dls::math::vec3d const &nor);

/* ************************************************************************** */

// return a random direction on the hemisphere
dls::math::vec3d cosine_direction(GNA &gna, dls::math::vec3d const &nor);

dls::math::vec3d get_brdf_ray(GNA &gna, dls::math::vec3d const &nor, dls::math::vec3d const &rd);

}  /* namespace kdo */
