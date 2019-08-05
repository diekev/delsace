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

#include "types.h"

class GNA;
class Texture;

namespace kdo {

class Lumiere;
class Maillage;
class Nuanceur;
struct ParametresRendu;

struct Objet;

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

struct Scene {
	Monde monde{};

	dls::tableau<Lumiere *> lumieres{};
	dls::tableau<Maillage *> maillages{};

	dls::tableau<Objet *> objets{};
	Objet *objet_actif = nullptr;

	Scene();
	~Scene();

	Scene(Scene const &) = default;
	Scene &operator=(Scene const &) = default;

	void ajoute_maillage(Maillage *maillage);
	void ajoute_lumiere(Lumiere *lumiere);
};

/* ************************************************************************** */

dls::math::vec3d normale_scene(Scene const &scene, dls::math::point3d const &position, dls::phys::esectd const &entresection);

double ombre_scene(ParametresRendu const &parametres, Scene const &scene, dls::phys::rayond const &rayon, double distance_maximale);

Spectre spectre_lumiere(ParametresRendu const &parametres, Scene const &scene, GNA &gna, dls::math::point3d const &pos, dls::math::vec3d const &nor);

/* ************************************************************************** */

// return a random direction on the hemisphere
dls::math::vec3d cosine_direction(GNA &gna, dls::math::vec3d const &nor);

dls::math::vec3d get_brdf_ray(GNA &gna, dls::math::vec3d const &nor, dls::math::vec3d const &rd);

}  /* namespace kdo */
