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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "manipulable.h"

namespace danjo {

enum {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

struct DonneesControle {
	void *pointeur = nullptr;
	dls::chaine nom = "";
	dls::chaine valeur_min = "";
	dls::chaine valeur_max = "";
	dls::chaine valeur_defaut = "";
	dls::chaine precision = "";
	dls::chaine pas = "";
	dls::chaine infobulle = "";
	dls::chaine filtres = "";
	dls::chaine suffixe = "";
	dls::tableau<std::pair<dls::chaine, dls::chaine>> valeur_enum{};
	TypePropriete type = {};
	etat_propriete etat = etat_propriete::VIERGE;

	bool initialisation = false;

	DonneesControle() = default;
	~DonneesControle() = default;

	DonneesControle(DonneesControle const &) = default;
	DonneesControle &operator=(DonneesControle const &) = default;
};

}  /* namespace danjo */
