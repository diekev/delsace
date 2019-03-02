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

#include <random>

#include <delsace/math/vecteur.hh>

class GNA {
	std::mt19937 m_gna;

public:
	explicit GNA(int graine = 1);

	/* distribution uniforme */

	int uniforme(int min, int max);

	long uniforme(long min, long max);

	float uniforme(float min, float max);

	double uniforme(double min, double max);

	dls::math::vec3f uniforme_vec3(float min, float max);

	/* distribution normale */

	float normale(float moyenne, float ecart);

	double normale(double moyenne, double ecart);

	dls::math::vec3f normale_vec3(float moyenne, float ecart);
};
