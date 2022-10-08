/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct AdaptriceTerrain;
struct ParametresErosionVent;
struct ParametresInclinaisonTerrain;
struct ParametresFiltrageTerrain;
struct ParametresErosionComplexe;
struct ParametresErosionSimple;
struct ParametresProjectionTerrain;
struct Maillage;

namespace geo {

void simule_erosion_vent(ParametresErosionVent &params,
                         AdaptriceTerrain &terrain,
                         AdaptriceTerrain *terrain_pour_facteur);

void incline_terrain(ParametresInclinaisonTerrain const &params, AdaptriceTerrain &terrain);

void filtrage_terrain(ParametresFiltrageTerrain const &params, AdaptriceTerrain &terrain);

void erosion_simple(ParametresErosionSimple const &params,
                    AdaptriceTerrain &terrain,
                    AdaptriceTerrain *grille_poids);

void erosion_complexe(ParametresErosionComplexe &params, AdaptriceTerrain &terrain);

void projette_geometrie_sur_terrain(ParametresProjectionTerrain const &params,
                                    AdaptriceTerrain &terrain,
                                    Maillage const &geometrie);

}  // namespace geo
