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

#include "fichier.hh"

#include <fstream>
#include <filesystem>

namespace dls {

chaine contenu_fichier(chaine const &chemin)
{
	if (!std::filesystem::exists(chemin.c_str())) {
		std::cerr << "Le fichier '" << chemin << "' n'existe pas !\n";
		return "";
	}

	std::ifstream entree;
	entree.open(chemin.c_str());

	if (!entree.is_open()) {
		std::cerr << "Le fichier '" << chemin << "' ne peut être ouvert !\n";
		return "";
	}

	return {(std::istreambuf_iterator<char>(entree)), (std::istreambuf_iterator<char>())};
}

}  /* namespace dls */
