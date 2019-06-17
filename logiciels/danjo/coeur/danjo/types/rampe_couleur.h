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

#include <vector>

#include <delsace/phys/couleur.hh>

enum {
	ENTREPOLATION_RVB = 0,
	ENTREPOLATION_HSV = 1,
};

struct PointRampeCouleur {
	float position{};
	dls::phys::couleur32 couleur{};
	bool selectionne = false;
};

struct RampeCouleur {
	std::vector<PointRampeCouleur> points{};
	char entrepolation = ENTREPOLATION_RVB;
};

void cree_rampe_defaut(RampeCouleur &rampe);

void tri_points_rampe(RampeCouleur &rampe);

void ajoute_point_rampe(RampeCouleur &rampe, float x, const dls::phys::couleur32 &couleur);

PointRampeCouleur *trouve_point_selectionne(const RampeCouleur &rampe);

dls::phys::couleur32 evalue_rampe_couleur(const RampeCouleur &rampe, const float valeur);
