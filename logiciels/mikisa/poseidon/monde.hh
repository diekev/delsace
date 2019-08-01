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
#include "corps/volume.hh"

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

	Grille<int> *drapeaux = nullptr;
	Grille<float> *densite = nullptr;
	Grille<float> *pression = nullptr;
	GrilleMAC *velocite = nullptr;

	bruit_vaguelette bruit{};

	dls::tableau<Particule *> particules{};
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

	~Poseidon();

	void supprime_particules();
};

void ajourne_sources(Poseidon &poseidon, int temps);

void ajourne_obstables(Poseidon &poseidon);

void fill_grid(Grille<int> &flags, int type);

}  /* namespace psn */
