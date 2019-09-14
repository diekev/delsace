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

#include "scene.hh"

namespace vision {
class Camera3D;
}  /* namespace vision */

namespace kdo {

class MoteurRendu;
class StructureAcceleration;

struct ParametresRendu {
	unsigned int nombre_echantillons = 32;
	unsigned int nombre_rebonds = 5;
	unsigned int resolution = 0;
	unsigned int hauteur_carreau = 32;
	unsigned int largeur_carreau = 32;
	double biais_ombre = 1e-4;

	Scene scene{};

#ifdef NOUVELLE_CAMERA
	CameraPerspective *camera = nullptr;
#else
	vision::Camera3D *camera = nullptr;
#endif

	ParametresRendu();
	~ParametresRendu();

	ParametresRendu(ParametresRendu const &) = delete;
	ParametresRendu &operator=(ParametresRendu const &) = delete;
};

struct InformationsRendu {
	/* Le temps écoulé depuis le début du rendu. */
	double temps_ecoule = 0.0;

	/* Le temps restant estimé pour finir le rendu. */
	double temps_restant = 0.0;

	/* Le temps pris pour calculer le dernier échantillon tiré. */
	double temps_echantillon = 0.0;

	/* L'échantillon courant. */
	unsigned int echantillon = 0;
};

struct Koudou {
	MoteurRendu *moteur_rendu;
	ParametresRendu parametres_rendu{};

	InformationsRendu informations_rendu{};

	vision::Camera3D *camera;

	Koudou();

	Koudou(Koudou const &) = delete;
	Koudou &operator=(Koudou const &) = delete;

	~Koudou();
};

}  /* namespace kdo */
