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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "iteration.hh"

namespace wlk {

template <typename T, typename type_tuile>
auto floute_volume(grille_eparse<T, type_tuile> const &grille_entree, int taille_fenetre)
{
    auto grille = memoire::loge<wlk::grille_eparse<T>>("wlk::grille_eparse", grille_entree.desc());
    grille->assure_tuiles(grille_entree.desc().etendue);

    wlk::pour_chaque_tuile_parallele(grille_entree, [&](wlk::tuile_scalaire<T> const *tuile) {
        auto min_tuile = tuile->min / wlk::TAILLE_TUILE;
        auto idx_tuile = dls::math::calcul_index(min_tuile, grille_entree.res_tuile());
        auto tuile_b = grille->tuile_par_index(idx_tuile);

        auto index_tuile = 0;
        for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
            for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
                for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
                    auto pos_tuile = tuile->min;
                    pos_tuile.x += i;
                    pos_tuile.y += j;
                    pos_tuile.z += k;

                    auto valeur = 0.0f;
                    auto poids = 0.0f;

                    for (auto kk = k - taille_fenetre; kk < k + taille_fenetre; ++kk) {
                        for (auto jj = j - taille_fenetre; jj < j + taille_fenetre; ++jj) {
                            for (auto ii = i - taille_fenetre; ii < i + taille_fenetre; ++ii) {
                                valeur += grille_entree.valeur(ii, jj, kk);
                                poids += 1.0f;
                            }
                        }
                    }

                    tuile_b->donnees[index_tuile] = valeur / poids;
                }
            }
        }
    });

    grille->elague();

    return grille;
}

template <typename T, typename type_tuile>
auto affine_volume(grille_eparse<T, type_tuile> const &grille_entree,
                   int taille_fenetre,
                   T const &poids_affinage)
{
    auto grille = memoire::loge<wlk::grille_eparse<T>>("wlk::grille_eparse", grille_entree.desc());
    grille->assure_tuiles(grille_entree.desc().etendue);

    wlk::pour_chaque_tuile_parallele(grille_entree, [&](wlk::tuile_scalaire<T> const *tuile) {
        auto min_tuile = tuile->min / wlk::TAILLE_TUILE;
        auto idx_tuile = dls::math::calcul_index(min_tuile, grille_entree.res_tuile());
        auto tuile_b = grille->tuile_par_index(idx_tuile);

        auto index_tuile = 0;
        for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
            for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
                for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
                    auto pos_tuile = tuile->min;
                    pos_tuile.x += i;
                    pos_tuile.y += j;
                    pos_tuile.z += k;

                    auto valeur = 0.0f;
                    auto poids = 0.0f;

                    for (auto kk = k - taille_fenetre; kk < k + taille_fenetre; ++kk) {
                        for (auto jj = j - taille_fenetre; jj < j + taille_fenetre; ++jj) {
                            for (auto ii = i - taille_fenetre; ii < i + taille_fenetre; ++ii) {
                                valeur += grille_entree.valeur(ii, jj, kk);
                                poids += 1.0f;
                            }
                        }
                    }

                    auto valeur_orig = tuile->donnees[index_tuile];
                    auto valeur_grossiere = valeur / poids;
                    auto valeur_fine = valeur_orig - valeur_grossiere;
                    auto valeur_affinee = valeur_orig + valeur_fine * poids_affinage;

                    tuile_b->donnees[index_tuile] = valeur_affinee;
                }
            }
        }
    });

    grille->elague();

    return grille;
}

template <typename T>
auto affine_grille(grille_dense_3d<T> &grille, float rayon, float quantite)
{
    auto taille = static_cast<int>(rayon);
    auto grille_aux = wlk::grille_dense_3d<T>(grille.desc());
    grille_aux.copie_donnees(grille);

    wlk::pour_chaque_voxel_parallele(grille, [&](T const &v, long idx, int x, int y, int z) {
        INUTILISE(idx);

        if (v == T(0)) {
            return v;
        }

        auto res = T();
        auto poids = 0.0f;

        for (auto zz = z - taille; zz <= z + taille; ++zz) {
            for (auto yy = y - taille; yy <= y + taille; ++yy) {
                for (auto xx = x - taille; xx <= x + taille; ++xx) {
                    res += grille_aux.valeur(dls::math::vec3i(xx, yy, zz));
                    poids += 1.0f;
                }
            }
        }

        if (poids != 0.0f) {
            res /= poids;
        }

        auto valeur_fine = v - res;
        auto valeur_affinee = v + valeur_fine * quantite;

        /* les valeurs négatives rendent la simulation instable */
        if (valeur_affinee < T(0)) {
            return v;
        }

        return valeur_affinee;
    });
}

template <typename T>
auto dilate_grille(grille_dense_3d<T> &grille, int rayon, interruptrice *chef = nullptr)
{
    auto performe_erosion = [&](grille_dense_3d<T> const &temp, int x, int y, int z) {
        auto v0 = temp.valeur(dls::math::vec3i(x, y, z));

        for (auto zz = z - rayon; zz <= z + rayon; ++zz) {
            for (auto yy = y - rayon; yy <= y + rayon; ++yy) {
                for (auto xx = x - rayon; xx <= x + rayon; ++xx) {
                    auto const &v1 = temp.valeur(dls::math::vec3i(xx, yy, zz));

                    v0 = std::max(v0, v1);
                }
            }
        }

        return v0;
    };

    transforme_grille(grille, performe_erosion, chef);
}

template <typename T>
auto erode_grille(grille_dense_3d<T> &grille, int rayon, interruptrice *chef = nullptr)
{
    auto performe_erosion = [&](grille_dense_3d<T> const &temp, int x, int y, int z) {
        auto v0 = temp.valeur(dls::math::vec3i(x, y, z));

        for (auto zz = z - rayon; zz <= z + rayon; ++zz) {
            for (auto yy = y - rayon; yy <= y + rayon; ++yy) {
                for (auto xx = x - rayon; xx <= x + rayon; ++xx) {
                    auto const &v1 = temp.valeur(dls::math::vec3i(xx, yy, zz));

                    v0 = std::min(v0, v1);
                }
            }
        }

        return v0;
    };

    transforme_grille(grille, performe_erosion, chef);
}

} /* namespace wlk */
