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

#include "poseidon.h"

#include <QFileDialog>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/vision/camera.h"

#include "commandes/commandes_temps.h"
#include "commandes/commandes_vue3d.h"

#include "fluide.h"

Poseidon::Poseidon()
	: usine_commande(new UsineCommande)
	, repondant_commande(new RepondantCommande(usine_commande, this))
	, animation(false)
	, fluide(new Fluide)
	, camera(new vision::Camera3D(0, 0))
{}

Poseidon::~Poseidon()
{
	delete fluide;
	delete camera;
	delete repondant_commande;
	delete usine_commande;
}

void Poseidon::enregistre_commandes()
{
	enregistre_commandes_temps(this->usine_commande);
	enregistre_commandes_vue3d(this->usine_commande);
}
