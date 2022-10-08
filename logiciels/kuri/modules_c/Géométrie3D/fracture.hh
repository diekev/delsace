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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

struct ParametresFracture;

#include "biblinternes/structures/tableau.hh"

namespace geo {

class Maillage;

struct CelluleVoronoi {
    dls::tableau<double> verts{};
    dls::tableau<int> poly_totvert{};
    dls::tableau<int> poly_indices{};
    dls::tableau<int> voisines{};

    float centroid[3] = {0.0f, 0.0f, 0.0f};
    float volume = 0.0f;
    int index = 0;
    int totvert = 0;
    int totpoly = 0;
    int pad = 0;
};

void fracture_maillage_voronoi(const ParametresFracture &params,
                               Maillage const &maillage_a_fracturer,
                               Maillage const &maillage_points,
                               Maillage &maillage_sortie);

}  // namespace geo
