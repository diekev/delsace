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

#include "manipulable.h"

namespace danjo {

enum {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

struct DonneesControle {
	void *pointeur = nullptr;
	std::string nom = "";
	std::string valeur_min = "";
	std::string valeur_max = "";
	std::string valeur_defaut = "";
	std::string precision = "";
	std::string pas = "";
	std::string infobulle = "";
	std::string filtres = "";
	std::string suffixe = "";
	std::vector<std::pair<std::string, std::string>> valeur_enum{};
	TypePropriete type = {};

	bool initialisation = false;

	DonneesControle() = default;
	~DonneesControle() = default;
};

}  /* namespace danjo */
