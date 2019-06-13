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

#include "genetique.h"

#include <string>

namespace dls {
namespace chisei {

std::string chaine_aleatoire(std::mt19937 &generateur, size_t longueur)
{
	std::uniform_int_distribution<char> lettre_aleatoire(-128, 127);
	std::string chaine;
	chaine.resize(longueur);

	std::generate(chaine.begin(), chaine.end(), [&]{ return lettre_aleatoire(generateur); });

	return chaine;
}

}  /* namespace chisei */
}  /* namespace dls */
