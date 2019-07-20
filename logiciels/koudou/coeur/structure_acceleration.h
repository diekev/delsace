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

#include "types.h"

#include "biblinternes/structures/tableau.hh"

class Scene;

/* ************************************************************************** */

class StructureAcceleration {
public:
	virtual ~StructureAcceleration() = default;

	virtual Entresection entresecte(Scene const &scene, Rayon const &rayon, double distance_maximale) const;
};

/* ************************************************************************** */

class VolumeEnglobant final : public StructureAcceleration {
	static constexpr auto NOMBRE_NORMAUX_PLAN = 7;
	static const dls::math::vec3d NORMAUX_PLAN[NOMBRE_NORMAUX_PLAN];

	/* La struct Etendue contient les valeurs d_proche et d_eloignée pour chacun
	 * des 7 plans englobant le volume de l'objet. */
	struct Etendue {
		Etendue();

		double d[NOMBRE_NORMAUX_PLAN][2];

		bool entresecte(Rayon const &rayon, double *numerateur_precalcule, double *denominateur_precalcule, double &d_proche, double &d_eloigne, uint8_t &index_plan) const;
	};

	dls::tableau<Etendue> m_etendues{};

public:
	void construit(Scene const &scene);

	Entresection entresecte(Scene const &scene, Rayon const &rayon, double distance_maximale) const override;
};
