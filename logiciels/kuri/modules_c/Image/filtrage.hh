/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

struct AdaptriceImage;
struct IMG_ParametresAffinageImage;
struct IMG_ParametresDilatationImage;
struct IMG_ParametresFiltrageImage;
struct IMG_ParametresFiltreBilateralImage;
struct IMG_ParametresMedianImage;

namespace image {

void filtre_image(const IMG_ParametresFiltrageImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie);

void affine_image(const IMG_ParametresAffinageImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie);

void dilate_image(const IMG_ParametresDilatationImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie);

void erode_image(const IMG_ParametresDilatationImage &params,
                 const AdaptriceImage &entree,
                 AdaptriceImage &sortie);

void filtre_median_image(const IMG_ParametresMedianImage &params,
                         const AdaptriceImage &entree,
                         AdaptriceImage &sortie);

void filtre_bilateral_image(const IMG_ParametresFiltreBilateralImage &params,
                            const AdaptriceImage &entree,
                            AdaptriceImage &sortie);
}  // namespace image
