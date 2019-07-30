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

#include "outils_visualisation.hh"

#include "corps/corps.h"

void dessine_boite(
		Corps &corps,
		Attribut *attr_C,
		dls::math::vec3f const &min,
		dls::math::vec3f const &max,
		dls::math::vec3f const &couleur)
{
	dls::math::vec3f sommets[8] = {
		dls::math::vec3f(min.x, min.y, min.z),
		dls::math::vec3f(min.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, min.z),
		dls::math::vec3f(min.x, max.y, min.z),
		dls::math::vec3f(min.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, min.z),
	};

	long cotes[12][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 },
		{ 4, 5 },
		{ 5, 6 },
		{ 6, 7 },
		{ 7, 4 },
	};

	auto decalage = corps.points()->taille();

	for (int i = 0; i < 8; ++i) {
		corps.ajoute_point(sommets[i].x, sommets[i].y, sommets[i].z);

		if (attr_C) {
			attr_C->pousse(couleur);
		}
	}

	for (int i = 0; i < 12; ++i) {
		auto poly = Polygone::construit(&corps, type_polygone::OUVERT, 2);
		poly->ajoute_sommet(decalage + cotes[i][0]);
		poly->ajoute_sommet(decalage + cotes[i][1]);
	}
}
