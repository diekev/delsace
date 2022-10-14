/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct AdaptriceImage;
struct ParametresSimulationGrain;

namespace image {

void simule_grain(const ParametresSimulationGrain &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie);

}
