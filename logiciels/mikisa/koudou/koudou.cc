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

#include "koudou.hh"

#include "biblinternes/vision/camera.h"

#include "moteur_rendu.hh"
#include "structure_acceleration.hh"

namespace kdo {

ParametresRendu::ParametresRendu()
	: acceleratrice(new VolumeEnglobant())
{}

ParametresRendu::~ParametresRendu()
{
#ifdef NOUVELLE_CAMERA
	delete camera;
#endif

	delete acceleratrice;
}

Koudou::Koudou()
	: moteur_rendu(new MoteurRendu)
	, camera(nullptr)
{

//#ifdef NOUVELLE_CAMERA
//	double fenetre_ecran[4] = {
//		-1.0, 1.0,
//		-1.0, 1.0
//	};

//	auto camera = new CameraPerspective();
//	camera->fenetre_ecran(fenetre_ecran);
//	camera->champs_de_vue(60.0);
//	camera->ouverture_obturateur(0.0);
//	camera->fermeture_obturateur(1.0);
//	camera->distance_focale(5.0);
//	camera->rayon_lentille(0.0);
//	camera->position(dls::math::vec3d(0.0, 1.0, 5.0));
//	camera->rotation(dls::math::vec3d(0.0, 0.0, 0.0));
//	camera->pellicule(m_koudou.moteur_rendu->pointeur_pellicule());

//	camera->ajourne();
//#endif
//	camera->projection(vision::TypeProjection::PERSPECTIVE);
//	parametres_rendu.camera = camera;
}

Koudou::~Koudou()
{
	delete moteur_rendu;
}

}  /* namespace kdo */
