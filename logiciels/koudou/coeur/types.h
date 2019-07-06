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

#include "biblinternes/math/vecteur.hh"

/* ************************************************************************** */

class Maillage;

enum {
	OBJET_TYPE_AUCUN    = 0,
	OBJET_TYPE_LUMIERE  = 1,
	OBJET_TYPE_TRIANGLE = 2,
};

struct Entresection {
	size_t id = 0;
	size_t id_triangle = 0;
	int type_objet = OBJET_TYPE_AUCUN;
	double distance = 0.0;
	const Maillage *maillage = nullptr;

	Entresection() = default;

	Entresection(Entresection const &entresection);

	Entresection &operator=(Entresection const &entresection);

	Entresection(Entresection &&entresection);

	Entresection &operator=(Entresection &&entresection);
};

/* ************************************************************************** */

struct Rayon {
	dls::math::point3d origine{};
	dls::math::vec3d direction{};
	dls::math::vec3d inverse_direction{};

	double distance_min = 0.0;
	double distance_max = 0.0;
	double temps = 0.0;
};

