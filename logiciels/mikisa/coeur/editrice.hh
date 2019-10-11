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

#include "biblinternes/structures/tableau.hh"
#include "biblinternes/vision/camera.h"
#include "biblinternes/vision/camera_2d.h"

struct Graphe;
struct Mikisa;

/**
 * Architecture/Idées :
 * - l'interface est divisée en région qui possède plusieurs éditrices
 * - l'interface est dessinée via Qt (ou autre) et toute logique est délégué aux
 *   éditrices changées via des commandes
 * - une interface possède un pointeur vers son éditrice, et quand une commande
 *   est appelée depuis telle ou telle interface elle met son pointeur dans le
 *   contexte à être utilisé par les commandes
 */

struct Editrice {
	Mikisa &mikisa;

	Editrice(Mikisa &ptr)
		: mikisa(ptr)
	{}
};

struct Editrice2D : public Editrice {
	vision::Camera2D camera{};

	Editrice2D(Mikisa &ptr)
		: Editrice(ptr)
	{}
};

struct Editrice3D : public Editrice {
	vision::Camera3D camera;

	Editrice3D(Mikisa &ptr)
		: Editrice(ptr)
		, camera(0, 0)
	{}
};

struct EditriceGraphe : public Editrice {
	dls::chaine chemin{};
	Graphe *graphe = nullptr;
	dls::chaine mode{};

	EditriceGraphe(Mikisa &ptr)
		: Editrice(ptr)
	{}

	COPIE_CONSTRUCT(EditriceGraphe);
};

struct EditricePropriete : public Editrice {
	EditricePropriete(Mikisa &ptr)
		: Editrice(ptr)
	{}
};

struct EditriceArborescence : public Editrice {
	EditriceArborescence(Mikisa &ptr)
		: Editrice(ptr)
	{}
};

struct EditriceLigneTemps : public Editrice {
	EditriceLigneTemps(Mikisa &ptr)
		: Editrice(ptr)
	{}
};

struct Region {
	Mikisa &mikisa;
	dls::tableau<Editrice *> editrices{};
	Editrice *editrice_courante{};

	Region(Mikisa &ptr)
		: mikisa(ptr)
	{}

	COPIE_CONSTRUCT(Region);

	void ajoute_editrice();

	void supprime_editrice();
};
