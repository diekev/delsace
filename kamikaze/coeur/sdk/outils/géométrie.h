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

#pragma once

#include <delsace/math/vecteur.hh>

class Attribute;
class PointList;
class PolygonList;

inline dls::math::vec3f normale_triangle(
		dls::math::vec3f const &v0,
		dls::math::vec3f const &v1,
		dls::math::vec3f const &v2)
{
	auto const n0 = v0 - v1;
	auto const n1 = v2 - v1;

	return dls::math::produit_croix(n1, n0);
}

void calcule_normales(
		PointList const &points,
		PolygonList const &polygones,
		Attribute &normales,
		bool flip);

void calcule_boite_delimitation(
		PointList const &points,
		dls::math::vec3f &min,
		dls::math::vec3f &max);
