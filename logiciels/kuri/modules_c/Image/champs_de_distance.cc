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

struct CanalPourChampsDeDistance {
    int largeur = 0;
    int hauteur = 0;
    const float *donnees_entree = nullptr;
    float *donnees_sortie[2] = {};

    IMG_ParametresChampsDeDistance params;
};

inline int64_t calcule_index(CanalPourChampsDeDistance &image, int i, int j)
{
    return j * image.largeur + i;
}

inline float valeur_entree(CanalPourChampsDeDistance &image, int i, int j)
{
    if (i < 0 || i >= image.largeur) {
        return 0.0f;
    }

    if (j < 0 || j >= image.hauteur) {
        return 0.0f;
    }

    auto index = calcule_index(image, i, j);
    return image.donnees_entree[index];
}

inline float valeur_sortie(CanalPourChampsDeDistance &image, int i, int j)
{
    if (i < 0 || i >= image.largeur) {
        return 0.0f;
    }

    if (j < 0 || j >= image.hauteur) {
        return 0.0f;
    }

    auto index = calcule_index(image, i, j);
    return image.donnees_sortie[0][index];
}

template <typename TypeOperation>
void applique_fonction_sur_entree(CanalPourChampsDeDistance &image, TypeOperation &&op)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        for (int y = plage.begin(); y < plage.end(); ++y) {
            for (int x = 0; x < res_x; ++x) {
                auto index = calcule_index(image, x, y);
                image.donnees_sortie[0][index] = op(image.donnees_entree[index], x, y);
            }
        }
    });
}

template <typename TypeOperation>
void applique_fonction_sur_sortie(CanalPourChampsDeDistance &image, TypeOperation &&op)
{
    auto const res_x = image.largeur;
    auto const res_y = image.hauteur;

    boucle_parallele(tbb::blocked_range<int>(0, res_y), [&](tbb::blocked_range<int> const &plage) {
        for (int y = plage.begin(); y < plage.end(); ++y) {
            for (int x = 0; x < res_x; ++x) {
                auto index = calcule_index(image, x, y);
                image.donnees_sortie[0][index] = op(image.donnees_sortie[0][index], x, y);
            }
        }
    });
}

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

    auto distance = image.donnees_sortie[0];

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
    if (!image.params.emets_gradients) {
        for (auto l = 0; l < sy; ++l) {
            for (auto c = 0; c < sx; ++c, ++index) {
                if (image.donnees_entree[index] < image.params.iso) {
                    distance[index] = -distance[index];
                }
            }
        }
    }
    else {
        float *gradient_x = image.donnees_sortie[0];
        float *gradient_y = image.donnees_sortie[1];
        for (auto l = 0; l < sy; ++l) {
            for (auto c = 0; c < sx; ++c, ++index) {
                auto p = nearest_point[l][c];
                gradient_x[index] = static_cast<float>(p.x - c) /
                                    static_cast<float>(image.largeur);
                gradient_y[index] = static_cast<float>(p.y - l) /
                                    static_cast<float>(image.hauteur);
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

    float *donnees_sortie = image.donnees_sortie[0];

    auto index = 0l;
    for (auto l = 0; l < nl; ++l) {
        for (auto c = 0; c < nc; ++c) {
            donnees_sortie[index++] = max_dist;
            nearest_point[l][c] = math::vec2i{0, 0};
        }
    }

    auto const iso = image.params.iso;

    /* initialize immediate interior & exterior elements */
    for (auto l = 1; l < nl - 2; ++l) {
        for (auto c = 1; c < nc - 2; ++c) {
            const bool inside = valeur_entree(image, c, l) > iso;

            if ((valeur_entree(image, c - 1, l) > iso) != inside ||
                (valeur_entree(image, c + 1, l) > iso) != inside ||
                (valeur_entree(image, c, l - 1) > iso) != inside ||
                (valeur_entree(image, c, l + 1) > iso) != inside) {
                index = calcule_index(image, c, l);
                donnees_sortie[index] = 0.0f;
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

    if (!image.params.emets_gradients) {
        // normalise(image);
    }
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
        if (valeur <= image.params.iso) {
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
    const auto unité = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));

    /* We use two point grids: one which keeps track of the interior distances,
     * while the other, the exterior distances. */
    math::matrice_dyn<math::vec2<int>> grille1(dimensions);
    math::matrice_dyn<math::vec2<int>> grille2(dimensions);

    initialise(image, grille1, grille2);

    scan(grille1);
    scan(grille2);

    /* Compute signed distance field from grid1 & grid2, and make it unsigned. */

    if (image.params.emets_gradients) {
        float *gradient_x = image.donnees_sortie[0];
        float *gradient_y = image.donnees_sortie[1];
        for (auto l = 1, ye = sy - 1; l < ye; ++l) {
            for (auto c = 1, xe = sx - 1; c < xe; ++c) {
                const float dist1 = std::sqrt(static_cast<float>(distance_carre(grille1[l][c])));
                const float dist2 = std::sqrt(static_cast<float>(distance_carre(grille2[l][c])));
                dls::math::vec2<int> p;
                if (dist1 < dist2) {
                    p = grille1[l][c];
                }
                else {
                    p = grille2[l][c];
                }

                auto const index = calcule_index(image, c, l);
                gradient_x[index] = static_cast<float>(p.x - c) /
                                    static_cast<float>(image.largeur);
                gradient_y[index] = static_cast<float>(p.y - l) /
                                    static_cast<float>(image.hauteur);
            }
        }
    }
    else {
        float *donnees_sortie = image.donnees_sortie[0];
        for (auto l = 1, ye = sy - 1; l < ye; ++l) {
            for (auto c = 1, xe = sx - 1; c < xe; ++c) {
                const float dist1 = std::sqrt(static_cast<float>(distance_carre(grille1[l][c])));
                const float dist2 = std::sqrt(static_cast<float>(distance_carre(grille2[l][c])));
                const float dist = dist1 - dist2;

                auto const index = calcule_index(image, c, l);
                donnees_sortie[index] = dist * unité;
            }
        }
    }
}

} /* namespace ssedt */

namespace generique {

static float calcule_distance(CanalPourChampsDeDistance &phi, int x, int y, float h)
{
    float *donnees_phi = phi.donnees_sortie[0];
    auto a = std::min(donnees_phi[calcule_index(phi, x - 1, y)],
                      donnees_phi[calcule_index(phi, x + 1, y)]);
    auto b = std::min(donnees_phi[calcule_index(phi, x, y - 1)],
                      donnees_phi[calcule_index(phi, x, y + 1)]);
    auto xi = 0.0f;

    if (std::abs(a - b) >= h) {
        xi = std::min(a, b) + h;
    }
    else {
        xi = 0.5f * (a + b + std::sqrt(2.0f * h * h - (a - b) * (a - b)));
    }

    return std::min(donnees_phi[calcule_index(phi, x, y)], xi);
}

void genere_champs_de_distance(CanalPourChampsDeDistance &image)
{
    const int res_x = image.largeur;
    const int res_y = image.hauteur;

    auto h = std::min(1.0f / static_cast<float>(res_x), 1.0f / static_cast<float>(res_y));

    const int64_t nombre_de_pixels = int64_t(res_x) * int64_t(res_y);

    float *donnees_sortie = image.donnees_sortie[0];
    const auto sx = image.largeur;
    const auto sy = image.hauteur;
    const auto dimensions = math::Dimensions(math::Hauteur(sy), math::Largeur(sx));
    const auto unité = std::min(1.0f / static_cast<float>(sx), 1.0f / static_cast<float>(sy));

    /* We use two point grids: one which keeps track of the interior distances,
     * while the other, the exterior distances. */
    math::matrice_dyn<math::vec2<int>> grille1(dimensions);

    for (int i = 0; i < nombre_de_pixels; i++) {
        auto const valeur = image.donnees_entree[i] > image.params.iso ? 1.0f : 0.0f;
        donnees_sortie[i] = valeur;
    }

    /* bas-droite */
    for (int x = 1; x < res_x - 1; ++x) {
        for (int y = 1; y < res_y - 1; ++y) {
            auto const index = calcule_index(image, x, y);
            auto dist = calcule_distance(image, x, y, h);
            if (dist < donnees_sortie[index]) {
                grille1[y][x] = math::vec2<int>(x, y);
            }
            donnees_sortie[index] = dist;
        }
    }

    /* bas-gauche */
    for (int x = res_x - 2; x >= 1; --x) {
        for (int y = 1; y < res_y - 1; ++y) {
            auto const index = calcule_index(image, x, y);
            auto dist = calcule_distance(image, x, y, h);
            if (dist < donnees_sortie[index]) {
                grille1[y][x] = math::vec2<int>(x, y);
            }
            donnees_sortie[index] = dist;
        }
    }

    /* haut-gauche */
    for (int x = res_x - 2; x >= 1; --x) {
        for (int y = res_y - 2; y >= 1; --y) {
            auto const index = calcule_index(image, x, y);
            auto dist = calcule_distance(image, x, y, h);
            if (dist < donnees_sortie[index]) {
                grille1[y][x] = math::vec2<int>(x, y);
            }
            donnees_sortie[index] = dist;
        }
    }

    /* haut-droite */
    for (int x = 1; x < res_x - 1; ++x) {
        for (int y = res_y - 2; y >= 1; --y) {
            auto const index = calcule_index(image, x, y);
            auto dist = calcule_distance(image, x, y, h);
            if (dist < donnees_sortie[index]) {
                grille1[y][x] = math::vec2<int>(x, y);
            }
            donnees_sortie[index] = dist;
        }
    }

    /* indicate interior and exterior pixel */
    auto index = 0;
    if (!image.params.emets_gradients) {
        for (auto l = 0; l < sy; ++l) {
            for (auto c = 0; c < sx; ++c, ++index) {
                if (image.donnees_entree[index] <= image.params.iso) {
                    donnees_sortie[index] = -donnees_sortie[index];
                }
            }
        }
    }
    else {
        float *gradient_x = image.donnees_sortie[0];
        float *gradient_y = image.donnees_sortie[1];
        for (auto l = 0; l < sy; ++l) {
            for (auto c = 0; c < sx; ++c, ++index) {
                auto p = grille1[l][c];
                gradient_x[index] = static_cast<float>(p.x - c) /
                                    static_cast<float>(image.largeur);
                gradient_y[index] = static_cast<float>(p.y - l) /
                                    static_cast<float>(image.hauteur);
            }
        }
    }
}

} /* namespace generique */

template <typename TypeParametres>
auto extrait_canaux_et_cree_sorties(const AdaptriceImage &entree,
                                    AdaptriceImage &sortie,
                                    IMG_ParametresChampsDeDistance const &params)
{
    dls::tableau<CanalPourChampsDeDistance> canaux;

    DescriptionImage desc;
    entree.decris_image(&entree, &desc);

    auto const nombre_de_calques = entree.nombre_de_calques(&entree);

    /* Crée les calques de sorties. */
    for (auto i = 0; i < nombre_de_calques; i++) {
        auto const calque_entree = entree.calque_pour_index(&entree, i);

        char *ptr_nom;
        int64_t taille_nom;
        entree.nom_calque(&entree, calque_entree, &ptr_nom, &taille_nom);

        auto calque_sortie = sortie.cree_calque(&sortie, ptr_nom, taille_nom);

        auto const nombre_de_canaux = entree.nombre_de_canaux(&entree, calque_entree);

        const void *canal_entree = nullptr;
        if (nombre_de_canaux == 4) {
            canal_entree = entree.canal_pour_index(&entree, calque_entree, 3);
        }
        else {
            canal_entree = entree.canal_pour_index(&entree, calque_entree, 0);
        }

        auto const donnees_canal_entree = entree.donnees_canal_pour_lecture(&entree, canal_entree);

        CanalPourChampsDeDistance donnees;
        donnees.hauteur = desc.hauteur;
        donnees.largeur = desc.largeur;
        donnees.params = params;
        donnees.donnees_entree = donnees_canal_entree;

        /* Sortie pour le champs de distance, ou l'axe X des gradients. */
        auto canal_sortie = sortie.ajoute_canal(&sortie, calque_sortie, ptr_nom, taille_nom);
        auto donnees_canal_sortie = sortie.donnees_canal_pour_ecriture(&sortie, canal_sortie);

        donnees.donnees_sortie[0] = donnees_canal_sortie;

        if (params.emets_gradients) {
            /* Sortie pour l'axe Y des gradients. */
            canal_sortie = sortie.ajoute_canal(&sortie, calque_sortie, ptr_nom, taille_nom);
            donnees_canal_sortie = sortie.donnees_canal_pour_ecriture(&sortie, canal_sortie);

            donnees.donnees_sortie[1] = donnees_canal_sortie;
        }

        canaux.ajoute(donnees);
    }

    return canaux;
}

void genere_champs_de_distance(const IMG_ParametresChampsDeDistance &params,
                               const AdaptriceImage &entree,
                               AdaptriceImage &sortie)
{
    auto canaux = extrait_canaux_et_cree_sorties<float>(entree, sortie, params);

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
