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

#include "bibliotheques/audition/audition.h"
#include "bibliotheques/commandes/commande.h"

class BaseEditrice;
class FenetrePrincipale;
class RepondantCommande;
class UsineCommande;

namespace vision {

class Camera3D;

}  /* namespace vision */

struct Fluide;

struct Poseidon : public Audite {
	/* Interface utilisateur. */
	FenetrePrincipale *fenetre_principale = nullptr;
	BaseEditrice *widget_actif = nullptr;

	UsineCommande usine_commande;

	RepondantCommande *repondant_commande;

	bool animation;

	Fluide *fluide;
	vision::Camera3D *camera;

	Poseidon();
	~Poseidon();

	Poseidon(const Poseidon &autre) = default;
	Poseidon &operator=(const Poseidon &autre) = default;

	void enregistre_commandes();
};

