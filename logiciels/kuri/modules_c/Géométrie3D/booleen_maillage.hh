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

bool construit_maillage_pour_cellules_voronoi(Maillage const &maillage_a,
                                              dls::tableau<CelluleVoronoi> const &cellules,
                                              const ParametresFracture &params,
                                              Maillage &maillage_sortie);

void subdivise_polyedre(EnrichedPolyhedron &polyedre);

void convertis_vers_maillage(EnrichedPolyhedron &polyhedre, Maillage &maillage);

}  // namespace geo
