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

#include "bibliotheques/spectre/spectre.h"
#include "bibliotheques/transformation/transformation.h"

class Nuanceur;

enum {
	LUMIERE_POINT,
	LUMIERE_DISTANTE,
};

struct Lumiere {
	math::transformation transformation;
	Nuanceur *nuanceur = nullptr;
	Spectre spectre;
	double intensite = 0.0f;
	char type = LUMIERE_POINT;

	Lumiere() = default;

	explicit Lumiere(const math::transformation &transform);

	virtual ~Lumiere();
};

/**
 * La struct LumierePoint représente une source de lumière sphérique (ampoule).
 * Dans ce cas, seule sa position dans la scène importe.
 */
struct LumierePoint final : public Lumiere {
	numero7::math::point3d pos;

	LumierePoint(const math::transformation &transform, Spectre spectre = Spectre(1.0), double intensite = 1.0);
};

/**
 * La struct LumiereDistante représente une source de lumière éloignée (Soleil).
 * Dans ce cas, la source est si loin que seule sa direction importe.
 */
struct LumiereDistante final : public Lumiere {
	numero7::math::vec3d dir;

	LumiereDistante(const math::transformation &transform, Spectre spectre = Spectre(1.0), double intensite = 1.0);
};
