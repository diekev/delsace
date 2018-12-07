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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <string>
#include <vector>

#include "melange.h"

enum TypeDonnees {
	SCALAIRE = 0,
	COULEUR  = 1,
	VECTEUR  = 2,
};

enum class TypeCalque {
	PEINTURE,
	PROCEDUREL,
	REPETABLE,
};

enum {
	CALQUE_ACTIF = 1 << 0,
};

struct Calque {
	TypeDonnees type_donnees = TypeDonnees::SCALAIRE;
	void *tampon = nullptr;
	int drapeaux = 0;

	std::string nom = "";

	TypeCalque type_calque = TypeCalque::PEINTURE;

	TypeMelange mode_fusion = TypeMelange::NORMAL;
	float opacite = 1.0f;

	/* TypeCalque::REPETABLE */
	float taille_u = 1.0f;
	float taille_v = 1.0f;
	std::string chemin = "";

	/* TypeCalque::PROCEDUREL */
	int octaves = 1;
	float taille = 1.0f;
	dls::math::vec4f couleur = dls::math::vec4f(0.0f);

	Calque() = default;
	~Calque();

	Calque(const Calque &autre) = default;
	Calque &operator=(const Calque &autre) = default;
};

enum TypeCanal {
	DIFFUSION,
	SPECULARITE,
	GLOSS,
	INCANDESCENCE,
	OPACITE,
	DEPLACEMENT_VECTORIEL,
	RELIEF,
	NORMAL,
	MASQUE_REFLECTION,

	NOMBRE_CANAUX
};

struct CanauxTexture {
	std::vector<Calque *> calques[TypeCanal::NOMBRE_CANAUX];

	/* La hauteur initiale du tampon des calques. */
	size_t hauteur{};

	/* La largeur initiale du tampon des calques. */
	size_t largeur{};

	dls::math::vec4f *tampon_diffusion = nullptr;

	CanauxTexture() = default;
	~CanauxTexture();

	CanauxTexture(const CanauxTexture &autre) = default;
	CanauxTexture &operator=(const CanauxTexture &autre) = default;
};

Calque *ajoute_calque(CanauxTexture &canaux, TypeCanal type_canal);

void supprime_calque(CanauxTexture &canaux, Calque *calque);

void fusionne_calques(CanauxTexture &canaux);
