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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/phys/rayon.hh"
#include "biblinternes/structures/file.hh"
#include "structures/tableau.hh"

typedef unsigned char axis_t;

struct HierarchieBoiteEnglobante {
    struct Noeud {
        float *limites = nullptr;
        Noeud **enfants = nullptr;
        struct Noeud *parent = nullptr;
        int index = 0;
        char nombre_enfants = 0;
        char axe_principal = 0;
        char niveau = 0;
    };

    float epsilon{};
    int tree_type{};
    axis_t axis{};
    axis_t start_axis{};
    axis_t stop_axis{};
    int totleaf{};
    int totbranch{};

    kuri::tableau<Noeud *> nodes{};
    kuri::tableau<float> nodebv{};
    kuri::tableau<Noeud *> nodechild{};
    kuri::tableau<Noeud> nodearray{};
};

namespace geo {

class Maillage;

HierarchieBoiteEnglobante *cree_hierarchie_boite_englobante(Maillage const &maillage);

/* ************************************* */

void visualise_hierarchie_au_niveau(HierarchieBoiteEnglobante &hierarchie,
                                    int niveau,
                                    Maillage &maillage);

/* ************************************* */

template <typename T>
auto fast_ray_nearest_hit(HierarchieBoiteEnglobante::Noeud const *node,
                          int *index,
                          dls::phys::rayon<T> const &ray,
                          dls::math::vec3<T> const &idot_axis,
                          T dist)
{
    auto t1x = (static_cast<T>(node->limites[index[0]]) - ray.origine[0]) * idot_axis[0];
    auto t2x = (static_cast<T>(node->limites[index[1]]) - ray.origine[0]) * idot_axis[0];
    auto t1y = (static_cast<T>(node->limites[index[2]]) - ray.origine[1]) * idot_axis[1];
    auto t2y = (static_cast<T>(node->limites[index[3]]) - ray.origine[1]) * idot_axis[1];
    auto t1z = (static_cast<T>(node->limites[index[4]]) - ray.origine[2]) * idot_axis[2];
    auto t2z = (static_cast<T>(node->limites[index[5]]) - ray.origine[2]) * idot_axis[2];

    if ((t1x > t2y || t2x < t1y || t1x > t2z || t2x < t1z || t1y > t2z || t2y < t1z) ||
        (t2x < 0.0 || t2y < 0.0 || t2z < 0.0) || (t1x > dist || t1y > dist || t1z > dist)) {
        return constantes<T>::INFINITE;
    }

    return std::max(t1x, std::max(t1y, t1z));
}

struct BVHElement {
    HierarchieBoiteEnglobante::Noeud const *noeud = nullptr;
    double t = 0.0;
};

inline bool operator<(BVHElement const &p1, BVHElement const &p2)
{
    return p1.t < p2.t;
}

template <typename TypeDelegue>
auto traverse(HierarchieBoiteEnglobante *tree,
              TypeDelegue const &delegue,
              dls::phys::rayond const &rayon)
{
    /* précalcule quelque données */
    auto ray_dot_axis = dls::math::vec3d();
    auto idot_axis = dls::math::vec3d();
    int index[6];

    static const dls::math::vec3d kdop_axes[3] = {dls::math::vec3d(1.0, 0.0, 0.0),
                                                  dls::math::vec3d(0.0, 1.0, 0.0),
                                                  dls::math::vec3d(0.0, 0.0, 1.0)};

    for (auto i = 0ul; i < 3; i++) {
        ray_dot_axis[i] = produit_scalaire(rayon.direction, kdop_axes[i]);
        idot_axis[i] = 1.0 / ray_dot_axis[i];

        if (std::abs(ray_dot_axis[i]) < 1e-6) {
            ray_dot_axis[i] = 0.0;
        }

        index[2 * i] = idot_axis[i] < 0.0 ? 1 : 0;
        index[2 * i + 1] = 1 - index[2 * i];
        index[2 * i] += 2 * static_cast<int>(i);
        index[2 * i + 1] += 2 * static_cast<int>(i);
    }

    auto const &racine = tree->nodes[tree->totleaf];

    auto dist_max = constantes<double>::INFINITE / 2.0;

    // auto distance_courant = racine.test_intersection_rapide(rayon);
    auto distance_courant = fast_ray_nearest_hit(racine, index, rayon, idot_axis, dist_max);
    auto esect = dls::phys::esectd{};

    if (distance_courant > dist_max) {
        return esect;
    }

    auto t_proche = rayon.distance_max;

    auto file = dls::file_priorite<BVHElement>();
    file.enfile({racine, 0.0});

    while (!file.est_vide()) {
        auto const noeud = file.defile().noeud;

        if (noeud->nombre_enfants == 0) {
            auto intersection = delegue.intersecte_element(noeud->index, rayon);

            if (!intersection.touche) {
                continue;
            }

            if (intersection.distance < t_proche) {
                t_proche = intersection.distance;
                esect = intersection;
            }
        }
        else {
            if (ray_dot_axis[static_cast<size_t>(noeud->axe_principal)] > 0.0) {
                for (auto i = 0; i < noeud->nombre_enfants; ++i) {
                    auto enfant = noeud->enfants[i];

                    distance_courant = fast_ray_nearest_hit(
                        enfant, index, rayon, idot_axis, dist_max);

                    if (distance_courant < dist_max) {
                        file.enfile({enfant, distance_courant});
                    }
                }
            }
            else {
                for (auto i = noeud->nombre_enfants - 1; i >= 0; --i) {
                    auto enfant = noeud->enfants[i];

                    distance_courant = fast_ray_nearest_hit(
                        enfant, index, rayon, idot_axis, dist_max);

                    if (distance_courant < dist_max) {
                        file.enfile({enfant, distance_courant});
                    }
                }
            }
        }
    }

    return esect;
}

}  // namespace geo
