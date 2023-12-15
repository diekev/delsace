/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct ParametreDistributionParticules;
struct ParametresDistributionPoisson2D;

namespace geo {

class Maillage;

void distribue_particules_sur_surface(ParametreDistributionParticules const &params,
                                      Maillage const &surface,
                                      Maillage &points_resultants);

void distribue_poisson_2d(ParametresDistributionPoisson2D const &params,
                          Maillage &points_resultants);

void construit_maillage_alpha(Maillage const &points,
                              const float rayon,
                              Maillage &maillage_résultat);

}  // namespace geo
