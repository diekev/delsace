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

#include "biblinternes/math/vecteur.hh"

#include "biblinternes/structures/tableau.hh"

class Corps;
class GroupePrimitive;

struct Triangle {
	using type_vec = dls::math::vec3f;

	type_vec v0 = type_vec(0.0f, 0.0f, 0.0f);
	type_vec v1 = type_vec(0.0f, 0.0f, 0.0f);
	type_vec v2 = type_vec(0.0f, 0.0f, 0.0f);

	long index_orig = 0;

	float aire = 0.0f;
	Triangle *precedent = nullptr, *suivant = nullptr;

	Triangle() = default;

	Triangle(type_vec const &v_0, type_vec const &v_1, type_vec const &v_2);
};

float calcule_aire(Triangle const &triangle);

dls::tableau<Triangle> convertis_maillage_triangles(Corps const *corps_entree, GroupePrimitive *groupe);
