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

#include "koudou.h"

#include <QFileDialog>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/vision/camera.h"

#include "configuration.h"
#include "moteur_rendu.h"
#include "structure_acceleration.h"

#include "commandes/commandes_fichier.h"
#include "commandes/commandes_rendu.h"
#include "commandes/commandes_vue3d.h"

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
	: camera(new vision::Camera3D(0, 0))
{
	moteur_rendu = new MoteurRendu;
	configuration = new Configuration;
	parametres_projet = new ParametresProjet;
	widget_actif = nullptr;
	usine_commande = new UsineCommande;
	repondant_commande = new RepondantCommande(usine_commande, this);

#ifdef NOUVELLE_CAMERA
	double fenetre_ecran[4] = {
		-1.0, 1.0,
		-1.0, 1.0
	};

	auto camera = new CameraPerspective();
	camera->fenetre_ecran(fenetre_ecran);
	camera->champs_de_vue(60.0);
	camera->ouverture_obturateur(0.0);
	camera->fermeture_obturateur(1.0);
	camera->distance_focale(5.0);
	camera->rayon_lentille(0.0);
	camera->position(numero7::math::vec3d(0.0, 1.0, 5.0));
	camera->rotation(numero7::math::vec3d(0.0, 0.0, 0.0));
	camera->pellicule(m_koudou.moteur_rendu->pointeur_pellicule());

	camera->ajourne();
#endif
	camera->projection(vision::TypeProjection::PERSPECTIVE);
	parametres_rendu.camera = camera;

	enregistre_commandes();
}

Koudou::~Koudou()
{
	delete usine_commande;
	delete repondant_commande;
	delete moteur_rendu;

	/* Le pointeur peut-être accédé par le destructeur des auditeurs lors de la
	 * fermeture du programme alors qu'il a déjà été détruit, donc on lui
	 * assigne la valeur nullptr pour proprement vérifier s'il est disponible.
	 */
	moteur_rendu = nullptr;

	delete configuration;
	delete parametres_projet;
	delete camera;
}

void Koudou::enregistre_commandes()
{
	enregistre_commandes_fichier(this->usine_commande);
	enregistre_commandes_rendu(this->usine_commande);
	enregistre_commandes_vue3d(this->usine_commande);
}

std::string Koudou::requiers_dialogue(int type)
{
	if (type == FICHIER_OUVERTURE) {
		const auto chemin = QFileDialog::getOpenFileName();
		return chemin.toStdString();
	}

	return "";
}
