/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau.hh"
#include "booleen/boolops_enriched_polyhedron.hpp"
#include <string>

struct ParametresFracture;

namespace geo {

class Maillage;
struct CelluleVoronoi;

bool booleen_maillages(Maillage const &maillage_a,
                       Maillage const &maillage_b,
                       const std::string &operation,
                       Maillage &maillage_sortie);

void test_conversion_polyedre(Maillage const &maillage_entree, Maillage &maillage_sortie);

bool construis_maillage_pour_cellules_voronoi(Maillage const &maillage_a,
                                              dls::tableau<CelluleVoronoi> const &cellules,
                                              const ParametresFracture &params,
                                              Maillage &maillage_sortie);

void subdivise_polyedre(EnrichedPolyhedron &polyedre);

void convertis_vers_maillage(EnrichedPolyhedron &polyhedre, Maillage &maillage);

}  // namespace geo
