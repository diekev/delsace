/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "champs_de_distance.hh"

#include <algorithm>
#include <cmath>

#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/outils.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/tableau.hh"

#include "application_fonction.hh"
#include "donnees_canal.hh"
#include "image.h"

using namespace dls;

namespace image {

using CanalPourChampsDeDistance = DonneesCanal<float>;

namespace navigation_estime {

/**
 * Génère un champs en utilisant un algorithme de navigation à l'estime.
 * Implémentation basé sur :
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.102.7988&rep=rep1&type=pdf
 */

void navigation_estime_ex(CanalPourChampsDeDistance &image,
                          math::matrice_dyn<math::vec2i> &nearest_point)
{
    const auto sx = image.largeur;
    const auto sy = image.hauteur;
    const auto unit = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));
    const auto diagonal = std::sqrt(2.0f) * unit;

    const auto decalage_x = 1;
    const auto decalage_y = sx;

    auto distance = image.donnees_sortie;

    /* Forward pass from top left to bottom right. */
    for (auto l = 1; l < sy - 1; ++l) {
        for (auto c = 1; c < sx - 1; ++c) {
            auto index = calcule_index(image, c, l);
            auto dist = distance[index];

            if (dist == 0.0f) {
                continue;
            }

            auto p = nearest_point[l][c];

            if (distance[index - decalage_x] + unit < dist) {
                p = nearest_point[l][c - 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index - decalage_x - decalage_y] + diagonal < dist) {
                p = nearest_point[l - 1][c - 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index - decalage_y] + unit < dist) {
                p = nearest_point[l - 1][c];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index - decalage_y + decalage_y] + diagonal < dist) {
                p = nearest_point[l - 1][c + 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            distance[index] = dist;
            nearest_point[l][c] = p;
        }
    }

    /* Backward pass from bottom right to top left. */
    for (auto l = sy - 2; l >= 1; --l) {
        for (auto c = sx - 2; c >= 1; --c) {
            auto index = calcule_index(image, c, l);
            auto dist = distance[index];

            if (dist == 0.0f) {
                continue;
            }

            auto p = nearest_point[l][c];

            if (distance[index + decalage_x] + unit < dist) {
                p = nearest_point[l][c + 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index + decalage_x + decalage_y] + diagonal < dist) {
                p = nearest_point[l + 1][c + 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index + decalage_y] + unit < dist) {
                p = nearest_point[l + 1][c];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            if (distance[index + decalage_y - decalage_x] + diagonal < dist) {
                p = nearest_point[l + 1][c - 1];
                dist = dls::math::hypotenuse(static_cast<float>(l - p[0]),
                                             static_cast<float>(c - p[1])) *
                       unit;
            }

            distance[index] = dist;
            nearest_point[l][c] = p;
        }
    }

    /* indicate interior and exterior pixel */
    auto index = 0;
    for (auto l = 0; l < sy; ++l) {
        for (auto c = 0; c < sx; ++c, ++index) {
            if (image.donnees_entree[index] < image.params) {
                distance[index] = -distance[index];
            }
        }
    }
}

void normalise(CanalPourChampsDeDistance &image)
{
    applique_fonction_sur_sortie(image, [&](float valeur, int x, int y) {
        float clamp = std::max(-1.0f, std::min(valeur, 1.0f));
        return ((clamp + 1.0f) * 0.5f);
    });

#if 0
    const size_t sx = field.width(), sy = field.height();
    size_t x, y;

    float s;
    for (y = 0; y < sy; ++y) {
        for (x = 0; x < sx; ++x) {
            s = field(x, y) * -1.0f;
            s = 1.0f - s;
            s *= s;
            s *= s;
            s *= s;
            field(x, y) = s;
        }
    }
#endif
}

void initialise(CanalPourChampsDeDistance &image, math::matrice_dyn<math::vec2i> &nearest_point)
{
    const auto nc = image.largeur;
    const auto nl = image.hauteur;
    const auto max_dist = dls::math::hypotenuse(static_cast<float>(nc), static_cast<float>(nl));

    auto index = 0l;
    for (auto l = 0; l < nl; ++l) {
        for (auto c = 0; c < nc; ++c) {
            image.donnees_sortie[index++] = max_dist;
            nearest_point[l][c] = math::vec2i{0, 0};
        }
    }

    /* initialize immediate interior & exterior elements */
    for (auto l = 1; l < nl - 2; ++l) {
        for (auto c = 1; c < nc - 2; ++c) {
            const bool inside = valeur_entree(image, c, l) > image.params;

            if ((valeur_entree(image, c - 1, l) > image.params) != inside ||
                (valeur_entree(image, c + 1, l) > image.params) != inside ||
                (valeur_entree(image, c, l - 1) > image.params) != inside ||
                (valeur_entree(image, c, l + 1) > image.params) != inside) {
                index = calcule_index(image, c, l);
                image.donnees_sortie[index] = 0.0f;
                nearest_point[l][c] = math::vec2i{l, c};
            }
        }
    }
}

void genere_champs_de_distance(CanalPourChampsDeDistance &image)
{
    const auto sx = image.largeur;
    const auto sy = image.hauteur;
    const auto dimensions = math::Dimensions(math::Hauteur(sy), math::Largeur(sx));
    math::matrice_dyn<math::vec2i> nearest_point(dimensions);

    initialise(image, nearest_point);
    navigation_estime_ex(image, nearest_point);

    normalise(image);
}

} /* namespace navigation_estime */

namespace ssedt {

/* 8-point Sequential Signed Euclidean Distance algorithm.
 * Similar to the Chamfer Distance Transform, 8SSED computes the distance field
 * by doing a double raster scan over the entire image. */

template <typename T>
T distance_carre(const math::vec2<T> &p)
{
    return p[0] * p[0] + p[1] * p[1];
}

inline void compare_dist(const math::matrice_dyn<math::vec2<int>> &g,
                         math::vec2<int> &p,
                         int c,
                         int l,
                         int decalage_c,
                         int decalage_l)
{
    math::vec2<int> other = g[l + decalage_l][c + decalage_c];
    other[0] += decalage_l;
    other[1] += decalage_c;

    if (distance_carre(other) < distance_carre(p)) {
        p = other;
    }
}

void scan(math::matrice_dyn<math::vec2<int>> &g)
{
    const auto sx = g.nombre_colonnes() - 1;
    const auto sy = g.nombre_lignes() - 1;

    /* Scan de haut en bas, de gauche à droite, en ne considérant que les
     * voisins suivant :
     * 1 1 1
     * 1 x -
     * - - -
     */
    for (int l = 1; l < sy; ++l) {
        for (auto c = 1; c < sx; ++c) {
            math::vec2<int> p = g[l][c];
            compare_dist(g, p, c, l, -1, 0);
            compare_dist(g, p, c, l, 0, -1);
            compare_dist(g, p, c, l, -1, -1);
            compare_dist(g, p, c, l, 1, -1);
            g[l][c] = p;
        }

        for (auto c = sx; c > 0; --c) {
            math::vec2<int> p = g[l][c];
            compare_dist(g, p, c, l, 1, 0);
            g[l][c] = p;
        }
    }

    /* Scan de bas en haut, de droite à gauche, en ne considérant que les
     * voisins suivant :
     * - - -
     * - x 1
     * 1 1 1
     */
    for (auto l = sy - 1; l > 0; --l) {
        for (auto c = sx - 1; c > 0; --c) {
            math::vec2<int> p = g[l][c];
            compare_dist(g, p, c, l, 1, 0);
            compare_dist(g, p, c, l, 0, 1);
            compare_dist(g, p, c, l, -1, 1);
            compare_dist(g, p, c, l, 1, 1);
            g[l][c] = p;
        }

        for (auto c = 1; c < sx; ++c) {
            math::vec2<int> p = g[l][c];
            compare_dist(g, p, c, l, -1, 0);
            g[l][c] = p;
        }
    }
}

void initialise(CanalPourChampsDeDistance &image,
                math::matrice_dyn<math::vec2<int>> &grille1,
                math::matrice_dyn<math::vec2<int>> &grille2)
{
    const auto sx = image.largeur;
    const auto sy = image.hauteur;
    const auto max_dist = static_cast<int>(
        dls::math::hypotenuse(static_cast<float>(sx), static_cast<float>(sy)));

    applique_fonction_sur_entree(image, [&](float valeur, int x, int y) {
        if (valeur < image.params) {
            grille1[y][x] = math::vec2i{0, 0};
            grille2[y][x] = math::vec2i{max_dist, max_dist};
        }
        else {
            grille1[y][x] = math::vec2i{max_dist, max_dist};
            grille2[y][x] = math::vec2i{0, 0};
        }

        return 0.0f;
    });
}

void genere_champs_de_distance(CanalPourChampsDeDistance &image)
{
    const auto sx = image.largeur;
    const auto sy = image.hauteur;
    const auto dimensions = math::Dimensions(math::Hauteur(sy), math::Largeur(sx));
    const auto unite = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));

    /* We use two point grids: one which keeps track of the interior distances,
     * while the other, the exterior distances. */
    math::matrice_dyn<math::vec2<int>> grille1(dimensions);
    math::matrice_dyn<math::vec2<int>> grille2(dimensions);

    initialise(image, grille1, grille2);

    scan(grille1);
    scan(grille2);

    /* Compute signed distance field from grid1 & grid2, and make it unsigned. */
    for (auto l = 1, ye = sy - 1; l < ye; ++l) {
        for (auto c = 1, xe = sx - 1; c < xe; ++c) {
            const float dist1 = std::sqrt(static_cast<float>(distance_carre(grille1[l][c])));
            const float dist2 = std::sqrt(static_cast<float>(distance_carre(grille2[l][c])));
            const float dist = dist1 - dist2;

            auto const index = calcule_index(image, c, l);
            image.donnees_sortie[index] = dist * unite;
        }
    }
}

} /* namespace ssedt */

namespace generique {

static float calcule_distance(DonneesCanal<float> &phi, int x, int y, float h)
{
    auto a = std::min(phi.donnees_sortie[calcule_index(phi, x - 1, y)],
                      phi.donnees_sortie[calcule_index(phi, x + 1, y)]);
    auto b = std::min(phi.donnees_sortie[calcule_index(phi, x, y - 1)],
                      phi.donnees_sortie[calcule_index(phi, x, y + 1)]);
    auto xi = 0.0f;

    if (std::abs(a - b) >= h) {
        xi = std::min(a, b) + h;
    }
    else {
        xi = 0.5f * (a + b + std::sqrt(2.0f * h * h - (a - b) * (a - b)));
    }

    return std::min(phi.donnees_sortie[calcule_index(phi, x, y)], xi);
}

void genere_champs_de_distance(DonneesCanal<float> &image)
{
    const int res_x = image.largeur;
    const int res_y = image.hauteur;

    auto h = std::min(1.0f / static_cast<float>(res_x), 1.0f / static_cast<float>(res_y));

    const long nombre_de_pixels = long(res_x) * long(res_y);

    for (int i = 0; i < nombre_de_pixels; i++) {
        auto const valeur = image.donnees_entree[i] > image.params ? 1.0f : 0.0f;
        image.donnees_sortie[i] = valeur;
    }

    /* bas-droite */
    for (int x = 1; x < res_x - 1; ++x) {
        for (int y = 1; y < res_y - 1; ++y) {
            auto const index = calcule_index(image, x, y);
            if (image.donnees_sortie[index] != 0.0f) {
                image.donnees_sortie[index] = calcule_distance(image, x, y, h);
            }
        }
    }

    /* bas-gauche */
    for (int x = res_x - 2; x >= 1; --x) {
        for (int y = 1; y < res_y - 1; ++y) {
            auto const index = calcule_index(image, x, y);
            if (image.donnees_sortie[index] != 0.0f) {
                image.donnees_sortie[index] = calcule_distance(image, x, y, h);
            }
        }
    }

    /* haut-gauche */
    for (int x = res_x - 2; x >= 1; --x) {
        for (int y = res_y - 2; y >= 1; --y) {
            auto const index = calcule_index(image, x, y);
            if (image.donnees_sortie[index] != 0.0f) {
                image.donnees_sortie[index] = calcule_distance(image, x, y, h);
            }
        }
    }

    /* haut-droite */
    for (int x = 1; x < res_x - 1; ++x) {
        for (int y = res_y - 2; y >= 1; --y) {
            auto const index = calcule_index(image, x, y);
            if (image.donnees_sortie[index] != 0.0f) {
                image.donnees_sortie[index] = calcule_distance(image, x, y, h);
            }
        }
    }
}

} /* namespace generique */

void genere_champs_de_distance(const IMG_ParametresChampsDeDistance &params,
                               const AdaptriceImage &entree,
                               AdaptriceImage &sortie)
{
    auto canaux = parse_canaux<float>(entree, sortie);

    for (auto &canal : canaux) {
        canal.params = params.iso;
    }

    for (auto &canal : canaux) {
        switch (params.methode) {
            case BALAYAGE_RAPIDE:
            {
                generique::genere_champs_de_distance(canal);
                break;
            }
            case NAVIGATION_ESTIME:
            {
                navigation_estime::genere_champs_de_distance(canal);
                break;
            }
            case DISTANCE_EUCLIDIENNE_SIGNEE_SEQUENTIELLE:
            {
                generique::genere_champs_de_distance(canal);
                break;
            }
        }
    }
}

}  // namespace image
