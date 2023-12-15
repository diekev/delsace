/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct Maillage;

namespace geo {

void calcule_enveloppe_convexe(const Maillage &maillage_pour, Maillage &résultat);

void triangulation_delaunay_2d_points_3d(Maillage const &points, Maillage &résultat);

}  // namespace geo
