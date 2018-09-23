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

#include <math/vec3.h>

#include <vector>

#include "bibliotheques/spectre/spectre.h"

#include "types.h"

class GNA;
class Lumiere;
class Maillage;
class Nuanceur;
class ParametresRendu;
class Texture;

struct Objet;

/* ************************************************************************** */

struct Monde {
	Texture *texture = nullptr;

	Monde() = default;

	~Monde();
};

Spectre spectre_monde(const Monde &monde, const numero7::math::vec3d &direction);

/* ************************************************************************** */

struct Scene {
	Monde monde;

	std::vector<Lumiere *> lumieres;
	std::vector<Maillage *> maillages;

	std::vector<Objet *> objets;
	Objet *objet_actif = nullptr;

	Scene();
	~Scene();

	void ajoute_maillage(Maillage *maillage);
	void ajoute_lumiere(Lumiere *lumiere);
};

/* ************************************************************************** */

numero7::math::vec3d normale_scene(const Scene &scene, const numero7::math::point3d &position, const Entresection &entresection);

double ombre_scene(const ParametresRendu &parametres, const Scene &scene, const Rayon &rayon, double distance_maximale);

Spectre spectre_lumiere(const ParametresRendu &parametres, const Scene &scene, GNA &gna, const numero7::math::point3d &pos, const numero7::math::vec3d &nor);

/* ************************************************************************** */

numero7::math::vec3d reflect(const numero7::math::vec3d &nor, const numero7::math::vec3d &dir);

// return a random direction on the hemisphere
numero7::math::vec3d cosine_direction(GNA &gna, const numero7::math::vec3d &nor);

numero7::math::vec3d get_brdf_ray(GNA &gna, const numero7::math::vec3d &nor, const numero7::math::vec3d &rd);
