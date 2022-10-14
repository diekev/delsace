/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/boucle.hh"

#include "donnees_canal.hh"

namespace image {

template <typename T, typename TypeOperation>
void applique_fonction_sur_entree(DonneesCanal<T> &image, TypeOperation &&op)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        for (int y = plage.begin(); y < plage.end(); ++y) {
            for (int x = 0; x < res_x; ++x) {
                auto index = calcule_index(image, x, y);
                image.donnees_sortie[index] = op(image.donnees_entree[index], x, y);
            }
        }
    });
}

template <typename T, typename TypeOperation>
void applique_fonction_sur_sortie(DonneesCanal<T> &image, TypeOperation &&op)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        for (int y = plage.begin(); y < plage.end(); ++y) {
            for (int x = 0; x < res_x; ++x) {
                auto index = calcule_index(image, x, y);
                image.donnees_sortie[index] = op(image.donnees_sortie[index], x, y);
            }
        }
    });
}

}  // namespace image
