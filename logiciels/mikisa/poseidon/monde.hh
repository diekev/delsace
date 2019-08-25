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

#pragma once

#include "biblinternes/structures/ensemble.hh"

#include "coeur/objet.h"

#include "wolika/grille_dense.hh"

#include "bruit_vaguelette.hh"
#include "particules.hh"

namespace psn {

enum mode_fusion {
	ADDITION,
	SOUSTRACTION,
	MULTIPLICATION,
	MINIMUM,
	MAXIMUM,
	SUPERPOSITION
};

struct ParametresSource {
	mode_fusion fusion{};
	float densite = 1.0f;
	float temperature = 1.0f;
	float fioul = 1.0f;
	float facteur = 1.0f;
	Objet *objet = nullptr;
	int debut = 0;
	int fin = 0;
};

struct Monde {
	dls::tableau<ParametresSource> sources{};
	dls::ensemble<Objet *> obstacles{};
};

struct Poseidon {
	Monde monde{};

	wlk::grille_dense_3d<int> *drapeaux = nullptr;
	wlk::grille_dense_3d<float> *densite = nullptr;
	wlk::grille_dense_3d<float> *temperature = nullptr;
	wlk::grille_dense_3d<float> *oxygene = nullptr;
	wlk::grille_dense_3d<float> *divergence = nullptr;
	wlk::grille_dense_3d<float> *fioul = nullptr;
	wlk::grille_dense_3d<float> *pression = nullptr;
	wlk::GrilleMAC *velocite = nullptr;

	/* pour la diffusion */
	wlk::grille_dense_3d<float> *densite_prev = nullptr;
	wlk::grille_dense_3d<float> *temperature_prev = nullptr;
	wlk::grille_dense_3d<float> *oxygene_prev = nullptr;
	wlk::GrilleMAC *velocite_prev = nullptr;

	bruit_vaguelette bruit{};

	particules parts{};
	GrilleParticule grille_particule{};

	float dt = 0.0f;
	float dt_min = 0.0f;
	float dt_max = 0.0f;
	float cfl = 0.0f;
	float duree_frame = 0.0f;
	float temps_par_frame = 0.0f;
	float temps_total = 0.0f;
	int image = 0;
	int resolution = 0;

	bool verrouille_dt = false;
	bool decouple = false;
	bool solveur_flip = false;

	~Poseidon();

	void supprime_particules();
};

void ajourne_sources(Poseidon &poseidon, int temps);

void ajourne_obstables(Poseidon &poseidon);

void fill_grid(wlk::grille_dense_3d<int> &flags, int type);

float calcul_vel_max(wlk::GrilleMAC const &vel);

void calcul_dt(Poseidon &poseidon, float vel_max);

}  /* namespace psn */
