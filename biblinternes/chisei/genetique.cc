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

#include "biblinternes/structures/chaine.hh"

namespace dls {
namespace chisei {

dls::chaine chaine_aleatoire(std::mt19937 &generateur, long longueur)
{
	std::uniform_int_distribution<char> lettre_aleatoire(-128, 127);
	dls::chaine chaine;
	chaine.redimensionne(longueur);

	std::generate(chaine.debut(), chaine.fin(), [&]{ return lettre_aleatoire(generateur); });

	return chaine;
}

}  /* namespace chisei */
}  /* namespace dls */
