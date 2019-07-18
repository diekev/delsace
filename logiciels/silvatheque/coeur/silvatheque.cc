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

#include "silvatheque.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/vision/camera.h"

#include "commandes/commandes_vue3d.h"

#include "arbre.h"

Silvatheque::Silvatheque()
	: usine_commande{}
	, repondant_commande(new RepondantCommande(usine_commande, this))
	, camera(new vision::Camera3D(0, 0))
	, arbre(new Arbre)
{}

Silvatheque::~Silvatheque()
{
	delete arbre;
	delete camera;
	delete repondant_commande;
}

void Silvatheque::enregistre_commandes()
{
	enregistre_commandes_vue3d(this->usine_commande);
}

