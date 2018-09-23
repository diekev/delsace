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

#include "bibliotheques/transformation/transformation.h"

#include <math/point3.h>
#include <math/vec3.h>

#include "corps/collection.h"

struct Objet {
	/* transformation */
	math::transformation transformation = math::transformation();
	numero7::math::point3f pivot        = numero7::math::point3f(0.0f);
	numero7::math::point3f position     = numero7::math::point3f(0.0f);
	numero7::math::point3f echelle      = numero7::math::point3f(1.0f);
	numero7::math::point3f rotation     = numero7::math::point3f(0.0f);
	float echelle_uniforme              = 1.0f;

	/* autres propriétés */
	std::string nom = "objet";

	Collection collection;

	Corps *corps;

	Objet() = default;
};
