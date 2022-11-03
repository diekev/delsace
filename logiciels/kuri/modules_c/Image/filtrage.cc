/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "filtrage.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/structures/tableau.hh"

#include "application_fonction.hh"
#include "donnees_canal.hh"

/* À FAIRE :
 * - OperatriceAnalyse
 * - OperatriceFiltrage (convolution)
 * - OperatriceNormalisationRegion
 * - OperatriceTournoiement
 * - OperatriceDeformation
 * - OperatriceCoordonneesPolaires
 * - OperatriceOndeletteHaar
 * - OperatriceExtractionPalette
 * - OperatricePrefiltreCubic
 * - OpRayonsSoleil
 * - OpLueurImage : requiers de traiter les canaux ensemble (RVB)
 * - OpMappageTonalOndelette
 */

namespace image {

/* ************************************************************************** */

template <typename T>
auto valeur_filtre_quadratic(T x)
{
    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _0_5 = static_cast<T>(0.5);
    constexpr auto _0_75 = static_cast<T>(0.75);
    constexpr auto _1_5 = static_cast<T>(1.5);

    if (x < _0) {
        x = -x;
    }

    if (x < _0_5) {
        return _0_75 - (x * x);
    }

    if (x < _1_5) {
        return _1_5 * (x - _1_5) * (x - _1_5);
    }

    return _0;
}

template <typename T>
auto valeur_filtre_cubic(T x)
{
    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _0_5 = static_cast<T>(0.5);
    constexpr auto _1 = static_cast<T>(1.0);
    constexpr auto _2 = static_cast<T>(2.0);
    constexpr auto _3 = static_cast<T>(3.0);
    constexpr auto _6 = static_cast<T>(6.0);

    auto x2 = x * x;

    if (x < _0) {
        x = -x;
    }

    if (x < _1) {
        return _0_5 * x * x2 - x2 + _2 / _3;
    }

    if (x < _2) {
        return (_2 - x) * (_2 - x) * (_2 - x) / _6;
    }

    return _0;
}

template <typename T>
auto valeur_filtre_catrom(T x)
{
    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _0_5 = static_cast<T>(0.5);
    constexpr auto _1 = static_cast<T>(1.0);
    constexpr auto _1_5 = static_cast<T>(1.5);
    constexpr auto _2 = static_cast<T>(2.0);
    constexpr auto _2_5 = static_cast<T>(2.5);
    constexpr auto _4 = static_cast<T>(4.0);

    auto x2 = x * x;

    if (x < _0) {
        x = -x;
    }

    if (x < _1) {
        return _1_5 * x2 * x - _2_5 * x2 + _1;
    }
    if (x < _2) {
        return -_0_5 * x2 * x + _2_5 * x2 - _4 * x + _2;
    }

    return _0;
}

/* bicubic de Mitchell & Netravali */
template <typename T>
auto valeur_filtre_mitchell(T x)
{
    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _1 = static_cast<T>(1.0);
    constexpr auto _2 = static_cast<T>(2.0);
    constexpr auto _3 = static_cast<T>(3.0);
    constexpr auto _6 = static_cast<T>(6.0);
    constexpr auto _8 = static_cast<T>(8.0);
    constexpr auto _9 = static_cast<T>(9.0);
    constexpr auto _12 = static_cast<T>(12.0);
    constexpr auto _18 = static_cast<T>(18.0);
    constexpr auto _24 = static_cast<T>(24.0);
    constexpr auto _30 = static_cast<T>(30.0);
    constexpr auto _48 = static_cast<T>(48.0);

    auto const b = _1 / _3;
    auto const c = _1 / _3;
    auto const p0 = (_6 - _2 * b) / _6;
    auto const p2 = (-_18 + _12 * b + _6 * c) / _6;
    auto const p3 = (_12 - _9 * b - _6 * c) / _6;
    auto const q0 = (_8 * b + _24 * c) / _6;
    auto const q1 = (-_12 * b - _48 * c) / _6;
    auto const q2 = (_6 * b + _30 * c) / _6;
    auto const q3 = (-b - _6 * c) / _6;

    if (x < -_2) {
        return 0.0f;
    }

    if (x < -_1) {
        return (q0 - x * (q1 - x * (q2 - x * q3)));
    }

    if (x < _0) {
        return (p0 + x * x * (p2 - x * p3));
    }

    if (x < _1) {
        return (p0 + x * x * (p2 + x * p3));
    }

    if (x < _2) {
        return (q0 + x * (q1 + x * (q2 + x * q3)));
    }

    return _0;
}

/* x va de -1 à 1 */
template <typename T>
auto valeur_filtre(IMG_TypeFiltre type, T x)
{
    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _1 = static_cast<T>(1.0);
    constexpr auto _2 = static_cast<T>(2.0);
    constexpr auto _3 = static_cast<T>(3.0);

    auto const gaussfac = static_cast<T>(1.6);

    x = std::abs(x);

    switch (type) {
        case TYPE_FILTRE_BOITE:
        {
            if (x > _1) {
                return _0;
            }

            return _1;
        }
        case TYPE_FILTRE_TRIANGULAIRE:
        {
            if (x > _1) {
                return _0;
            }

            return _1 - x;
        }
        case TYPE_FILTRE_GAUSSIEN:
        {
            const float two_gaussfac2 = _2 * gaussfac * gaussfac;
            x *= _3 * gaussfac;

            return _1 / std::sqrt(constantes<T>::PI * two_gaussfac2) *
                   std::exp(-x * x / two_gaussfac2);
        }
        case TYPE_FILTRE_MITCHELL:
        {
            return valeur_filtre_mitchell(x * gaussfac);
        }
        case TYPE_FILTRE_QUADRATIC:
        {
            return valeur_filtre_quadratic(x * gaussfac);
        }
        case TYPE_FILTRE_CUBIC:
        {
            return valeur_filtre_cubic(x * gaussfac);
        }
        case TYPE_FILTRE_CATROM:
        {
            return valeur_filtre_catrom(x * gaussfac);
        }
    }

    return _0;
}

template <typename T>
static void initialise_table_filtre(dls::tableau<T> &table, int taille, int n, IMG_TypeFiltre type, T rayon)
{
    table.redimensionne(n);

    /* nécessaire pour avoir une meilleure sûreté de type, sans avoir à écrire
     * static_cast partout */
    constexpr auto _0 = static_cast<T>(0);
    constexpr auto _1 = static_cast<T>(1.0);

    auto fac = (rayon > _0 ? _1 / rayon : _0);
    auto somme = _0;

    for (auto i = -taille; i <= taille; i++) {
        auto val = valeur_filtre(type, static_cast<T>(i) * fac);
        somme += val;
        table[i + taille] = val;
    }

    somme = 1.0f / somme;

    for (auto i = 0; i < n; i++) {
        table[i] *= somme;
    }
}

template <typename T>
static dls::tableau<T> cree_table_filtre(IMG_TypeFiltre type, T rayon)
{
    auto taille = static_cast<int>(rayon);
    auto n = 2 * taille + 1;

    auto table = dls::tableau<T>(n);
    initialise_table_filtre(table, taille, n, type, rayon);
    return table;
}

/* ************************************************************************** */

static float rayon_pour_filtre(IMG_ParametresFiltrageImage const &params)
{
    if (params.filtre == TYPE_FILTRE_GAUSSIEN) {
        return params.rayon * 2.57f;
    }
    return params.rayon;
}

using CanalPourFiltrage = DonneesCanal<IMG_ParametresFiltrageImage>;

void filtre_image(CanalPourFiltrage &image)
{
    auto const type = image.params.filtre;
    auto const rayon = rayon_pour_filtre(image.params);
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    auto table = cree_table_filtre(type, rayon);

    /* Applique filtre sur l'axe des X. */
    applique_fonction_sur_entree(image, [&](float /*valeur*/, int x, int y) {
        auto valeur = 0.0f;
        auto rayon_i = static_cast<int>(rayon);

        for (auto ix = x - rayon_i, k = 0; ix < x + rayon_i + 1; ix++, ++k) {
            auto p = valeur_entree(image, ix, y);
            valeur += p * table[k];
        }

        return valeur;
    });

    /* Applique le filtre sur l'axe des Y. Nous devons l'appliquer sur la sortie. */
    applique_fonction_sur_sortie_vertical(image, [&](float /*valeur*/, int x, int y) {
        auto valeur = 0.0f;
        auto rayon_i = static_cast<int>(rayon);

        for (auto iy = y - rayon_i, k = 0; iy < y + rayon_i + 1; iy++, ++k) {
            auto p = valeur_sortie(image, x, iy);
            valeur += p * table[k];
        }

        return valeur;
    });
}

void filtre_image(const IMG_ParametresFiltrageImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresFiltrageImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        filtre_image(canal);
    }
}

/* ************************************************************************** */

using CanalPourAffinage = DonneesCanal<IMG_ParametresAffinageImage>;

void affine_image(CanalPourAffinage &image)
{
    auto pour_filtrage = CanalPourFiltrage{};
    pour_filtrage.donnees_entree = image.donnees_entree;
    pour_filtrage.donnees_sortie = image.donnees_sortie;
    pour_filtrage.largeur = image.largeur;
    pour_filtrage.hauteur = image.hauteur;
    pour_filtrage.params.filtre = image.params.filtre;
    pour_filtrage.params.rayon = image.params.rayon;

    filtre_image(pour_filtrage);

    applique_fonction_sur_entree(image, [&](float valeur_entree, int x, int y) {
        auto valeur_grossiere = valeur_sortie(image, x, y);
        auto valeur_fine = valeur_entree - valeur_grossiere;
        return valeur_entree + valeur_fine * image.params.poids;
    });
}

void affine_image(const IMG_ParametresAffinageImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresAffinageImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        affine_image(canal);
    }
}

/* ************************************************************************** */

using CanalPourDilatation = DonneesCanal<IMG_ParametresDilatationImage>;

void dilate_image(CanalPourDilatation &image)
{
    auto performe_dilatation = [&](float /*pixel*/, int x, int y) {
        auto const rayon = image.params.rayon;
        auto const res_x = image.largeur;
        auto const res_y = image.hauteur;
        auto const debut_x = std::max(0, x - rayon);
        auto const debut_y = std::max(0, y - rayon);
        auto const fin_x = std::min(res_x, x + rayon);
        auto const fin_y = std::min(res_y, y + rayon);

        auto p0 = -std::numeric_limits<float>::max();

        for (int sy = debut_y; sy < fin_y; ++sy) {
            for (int sx = debut_x; sx < fin_x; ++sx) {
                auto p = valeur_entree(image, sx, sy);
                p0 = std::max(p0, p);
            }
        }

        return p0;
    };

    applique_fonction_sur_entree(image, performe_dilatation);
}

void erode_image(CanalPourDilatation &image)
{
    auto performe_erosion = [&](float /*pixel*/, int x, int y) {
        auto const rayon = image.params.rayon;
        auto const res_x = image.largeur;
        auto const res_y = image.hauteur;
        auto const debut_x = std::max(0, x - rayon);
        auto const debut_y = std::max(0, y - rayon);
        auto const fin_x = std::min(res_x, x + rayon);
        auto const fin_y = std::min(res_y, y + rayon);

        auto p0 = std::numeric_limits<float>::max();

        for (int sy = debut_y; sy < fin_y; ++sy) {
            for (int sx = debut_x; sx < fin_x; ++sx) {
                auto p = valeur_entree(image, sx, sy);
                p0 = std::min(p0, p);
            }
        }

        return p0;
    };

    applique_fonction_sur_entree(image, performe_erosion);
}

void dilate_image(const IMG_ParametresDilatationImage &params,
                  const AdaptriceImage &entree,
                  AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresDilatationImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        dilate_image(canal);
    }
}

void erode_image(const IMG_ParametresDilatationImage &params,
                 const AdaptriceImage &entree,
                 AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresDilatationImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        erode_image(canal);
    }
}

/* ************************************************************************** */

using CanalPourMedian = DonneesCanal<IMG_ParametresMedianImage>;

void filtre_median_image(CanalPourMedian &image)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        auto taille = image.params.rayon;
        auto taille_fenetre = (2 * taille + 1) * (2 * taille + 1);
        auto moitie_taille = taille_fenetre / 2;
        auto est_paire = taille_fenetre % 2 == 0;
        auto valeurs_R = dls::tableau<float>(taille_fenetre);

        for (int y = plage.begin(); y < plage.end(); ++y) {
            for (int x = 0; x < res_x; ++x) {
                auto index = calcule_index(image, x, y);
                auto index_v = 0;
                for (auto j = -taille; j <= taille; ++j) {
                    for (auto i = -taille; i <= taille; ++i, ++index_v) {
                        valeurs_R[index_v] = valeur_entree(image, x + i, y + j);
                    }
                }

                std::sort(begin(valeurs_R), end(valeurs_R));

                if (est_paire) {
                    image.donnees_sortie[index] = (valeurs_R[moitie_taille] +
                                                   valeurs_R[moitie_taille + 1]) *
                                                  0.5f;
                }
                else {
                    image.donnees_sortie[index] = valeurs_R[moitie_taille];
                }
            }
        }
    });
}

void filtre_median_image(const IMG_ParametresMedianImage &params,
                         const AdaptriceImage &entree,
                         AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresMedianImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        filtre_median_image(canal);
    }
}

/* ************************************************************************** */

using CanalPourFiltreBilateral = DonneesCanal<IMG_ParametresFiltreBilateralImage>;

void filtre_bilateral_image(CanalPourFiltreBilateral &image)
{
    auto gaussien = [](float x, float sigma) {
        auto sigma2 = sigma * sigma;
        return std::exp(-(x * x) / (2.0f * sigma2)) / (constantes<float>::PI * sigma2);
    };

    applique_fonction_sur_entree(image, [&](float source, int x, int y) {
        auto const taille = image.params.rayon;
        auto const sigma_s = image.params.sigma_s;
        auto const sigma_i = image.params.sigma_i;

        auto filtre = 0.0f;
        auto poids_r = 0.0f;

        for (auto j = -taille; j <= taille; ++j) {
            for (auto i = -taille; i <= taille; ++i) {
                auto const v = valeur_entree(image, x + i, y + j);
                auto const gir = gaussien(v - source, sigma_i);
                auto const gs = gaussien(std::sqrt(static_cast<float>(i * i + j * j)), sigma_s);
                auto const pr = gir * gs;

                filtre += v * pr;
                poids_r += pr;
            }
        }

        if (poids_r) {
            filtre /= poids_r;
        }

        return filtre;
    });
}

void filtre_bilateral_image(const IMG_ParametresFiltreBilateralImage &params,
                            const AdaptriceImage &entree,
                            AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<IMG_ParametresFiltreBilateralImage>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params;
    }

    for (auto &canal : canaux) {
        filtre_bilateral_image(canal);
    }
}

/* ************************************************************************** */

using CanalPourDefocalisation = DonneesCanal<float>;

static void defocalise_canal(CanalPourDefocalisation &image,
                             IMG_Fenetre &fenetre,
                             const float *rayon_flou_par_pixel)
{
    auto const min_x = fenetre.min_x;
    auto const max_x = fenetre.max_x + 1;
    auto const min_y = fenetre.min_y;
    auto const max_y = fenetre.max_y + 1;
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    /* Applique filtre sur l'axe des X. */
    boucle_parallele(
        tbb::blocked_range<int>(min_y, max_y), [&](tbb::blocked_range<int> const &plage) {
            dls::tableau<float> table;
            for (int y = plage.begin(); y < plage.end(); ++y) {
                for (int x = min_x; x < max_x; ++x) {
                    auto valeur = 0.0f;
                    auto rayon = rayon_flou_par_pixel[calcule_index(image, x, y)];
                    auto rayon_i = static_cast<int>(rayon);
                    initialise_table_filtre(
                        table, rayon_i, 2 * rayon_i + 1, TYPE_FILTRE_GAUSSIEN, rayon);

                    for (auto ix = x - rayon_i, k = 0; ix < x + rayon_i + 1; ix++, ++k) {
                        auto p = valeur_entree(image, ix, y);
                        valeur += p * table[k];
                    }

                    image.donnees_sortie[calcule_index(image, x, y)] = valeur;
                }
            }
        });

    /* Applique le filtre sur l'axe des Y. Nous devons l'appliquer sur la sortie. */
    boucle_parallele(
        tbb::blocked_range<int>(min_x, max_x), [&](tbb::blocked_range<int> const &plage) {
            dls::tableau<float> table;
            for (int x = plage.begin(); x < plage.end(); ++x) {
                for (int y = min_y; y < max_y; ++y) {
                    auto valeur = 0.0f;
                    auto rayon = rayon_flou_par_pixel[calcule_index(image, x, y)];
                    auto rayon_i = static_cast<int>(rayon);
                    initialise_table_filtre(
                        table, rayon_i, 2 * rayon_i + 1, TYPE_FILTRE_GAUSSIEN, rayon);

                    for (auto iy = y - rayon_i, k = 0; iy < y + rayon_i + 1; iy++, ++k) {
                        auto p = valeur_sortie(image, x, iy);
                        valeur += p * table[k];
                    }

                    image.donnees_sortie[calcule_index(image, x, y)] = valeur;
                }
            }
        });
}

void defocalise_image(const AdaptriceImage &entree,
                      AdaptriceImage &sortie,
                      IMG_Fenetre &fenetre,
                      const float *rayon_flou_par_pixel)
{
    auto canaux = parse_canaux<float>(entree, sortie);

    for (auto &canal : canaux) {
        defocalise_canal(canal, fenetre, rayon_flou_par_pixel);
    }
}

}  // namespace image
