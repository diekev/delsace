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

#include "kanba.h"

#include <QFileDialog>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/vision/camera.h"

#include "brosse.h"
#include "maillage.h"

#include "commandes/commandes_calques.h"
#include "commandes/commandes_fichier.h"
#include "commandes/commandes_vue2d.h"
#include "commandes/commandes_vue3d.h"

Kanba::Kanba()
	: tampon(numero7::math::Hauteur(1080), numero7::math::Largeur(1920))
	, usine_commande(new UsineCommande)
	, repondant_commande(new RepondantCommande(usine_commande, this))
	, brosse(new Brosse())
	, camera(new vision::Camera3D(0, 0))
	, maillage(nullptr)
{}

Kanba::~Kanba()
{
	delete brosse;
	delete maillage;
	delete camera;
	delete repondant_commande;
	delete usine_commande;
}

void Kanba::enregistre_commandes()
{
	enregistre_commandes_calques(this->usine_commande);
	enregistre_commandes_fichier(this->usine_commande);
	enregistre_commandes_vue2d(this->usine_commande);
	enregistre_commandes_vue3d(this->usine_commande);
}

std::string Kanba::requiers_dialogue(int type)
{
	if (type == FICHIER_OUVERTURE) {
		const auto chemin = QFileDialog::getOpenFileName();
		return chemin.toStdString();
	}

	return "";
}
