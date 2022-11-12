/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct AdaptriceImage;
struct IMG_ParametresChampsDeDistance;

namespace image {

void genere_champs_de_distance(const IMG_ParametresChampsDeDistance &params,
                               const AdaptriceImage &entree,
                               AdaptriceImage &sortie);

}
