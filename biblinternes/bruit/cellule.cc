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

#include "cellule.hh"

#include "biblinternes/outils/definitions.h"

#include "outils.hh"

namespace bruit {

static float cellNoiseU(float x, float y, float z)
{
	/* avoid precision issues on unit coordinates */
	x = (x + 0.000001f) * 1.00001f;
	y = (y + 0.000001f) * 1.00001f;
	z = (z + 0.000001f) * 1.00001f;

	auto xi = static_cast<unsigned int>(std::floor(x));
	auto yi = static_cast<unsigned int>(std::floor(y));
	auto zi = static_cast<unsigned int>(std::floor(z));
	auto n = xi + yi * 1301 + zi * 314159;
	n ^= (n << 13);
	return (static_cast<float>(n * (n * n * 15731 + 789221) + 1376312589) / 4294967296.0f);
}

void cellule::construit(parametres &params, int graine)
{
	construit_defaut(params, graine);
}

float cellule::evalue(parametres const &params, dls::math::vec3f pos)
{
	INUTILISE(params);
	return cellNoiseU(pos.x, pos.y, pos.z) * 2.0f -1.0f;
}

float cellule::evalue_derivee(const parametres &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	INUTILISE(params);
	INUTILISE(derivee);
	return cellNoiseU(pos.x, pos.y, pos.z) * 2.0f -1.0f;
}

}  /* namespace bruit */
