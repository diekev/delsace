/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

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
