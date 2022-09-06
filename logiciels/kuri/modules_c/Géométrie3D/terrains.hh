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
