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

#include "particules.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/outils/temps.hh"
#include "biblinternes/structures/grille_particules.hh"
#include "biblinternes/structures/tableau.hh"

#include "outils.hh"

#include <limits>

namespace geo {

/**
 * "Dart Throwing on Surfaces" (Cline et al. 2009)
 * http://peterwonka.net/Publications/pdfs/2009.EGSR.Cline.PoissonSamplingOnSurfaces.pdf
 */

/* La densité de l'arrangement de cercles ayant la plus grande densité, selon
 * Lagrange. C'est-à-dire le pourcentage d'aire qu'occuperait un arrangement de
 * cercle étant le plus compacte. */
static constexpr auto DENSITE_CERCLE = 0.9068996821171089f;

static constexpr auto NOMBRE_BOITE = 64;

/* ************************************************************************** */

struct Triangle {
    using type_vec = dls::math::vec3f;

    type_vec v0 = type_vec(0.0f, 0.0f, 0.0f);
    type_vec v1 = type_vec(0.0f, 0.0f, 0.0f);
    type_vec v2 = type_vec(0.0f, 0.0f, 0.0f);

    long index_orig = 0;

    float aire = 0.0f;
    Triangle *precedent = nullptr, *suivant = nullptr;

    Triangle() = default;

    Triangle(type_vec const &v_0, type_vec const &v_1, type_vec const &v_2)
        : v0(v_0), v1(v_1), v2(v_2)
    {
    }
};

static float calcule_aire(const Triangle &triangle)
{
    return calcule_aire(triangle.v0, triangle.v1, triangle.v2);
}

static dls::tableau<Triangle> convertis_maillage_triangles(Maillage const &surface, void *groupe)
{
    /* Convertis le maillage en triangles.
     * Petit tableau pour comprendre le calcul du nombre de triangles.
     * +----------------+------------------+
     * | nombre sommets | nombre triangles |
     * +----------------+------------------+
     * | 3              | 1                |
     * | 4              | 2                |
     * | 5              | 3                |
     * | 6              | 4                |
     * | 7              | 5                |
     * +----------------+------------------+
     */
    auto nombre_de_triangles = 0l;
    for (int i = 0; i < surface.nombreDePolygones(); i++) {
        nombre_de_triangles += surface.nombreDeSommetsPolygone(i) - 2;
    }

    dls::tableau<Triangle> triangles(nombre_de_triangles);
    auto index_triangle = 0;
    for (int i = 0; i < surface.nombreDePolygones(); i++) {
        Triangle triangle;
        surface.pointPourSommetPolygones(i, 0, triangle.v0);

        for (long j = 2; j < surface.nombreDeSommetsPolygone(i); ++j) {

            surface.pointPourSommetPolygones(i, j - 1, triangle.v1);
            surface.pointPourSommetPolygones(i, j, triangle.v2);
            triangle.index_orig = i;

            triangles[index_triangle++] = triangle;
        }
    }

    return triangles;
}

class ListeTriangle {
    Triangle *m_premier_triangle = nullptr;
    Triangle *m_dernier_triangle = nullptr;

  public:
    ListeTriangle() = default;

    ListeTriangle(ListeTriangle const &) = default;
    ListeTriangle &operator=(ListeTriangle const &) = default;

    ~ListeTriangle()
    {
        auto triangle = m_premier_triangle;
        while (triangle != nullptr) {
            auto tri_suiv = triangle->suivant;
            memoire::deloge("Triangle", triangle);
            triangle = tri_suiv;
        }
    }

    Triangle *ajoute(dls::math::vec3f const &v0,
                     dls::math::vec3f const &v1,
                     dls::math::vec3f const &v2)
    {
        auto triangle = memoire::loge<Triangle>("Triangle", v0, v1, v2);
        triangle->aire = calcule_aire(*triangle);
        triangle->precedent = nullptr;
        triangle->suivant = nullptr;

        if (m_premier_triangle == nullptr) {
            m_premier_triangle = triangle;
        }
        else {
            triangle->precedent = m_dernier_triangle;
            m_dernier_triangle->suivant = triangle;
        }

        m_dernier_triangle = triangle;

        return triangle;
    }

    void enleve(Triangle *triangle)
    {
        if (triangle->precedent) {
            triangle->precedent->suivant = triangle->suivant;
        }

        if (triangle->suivant) {
            triangle->suivant->precedent = triangle->precedent;
        }

        if (triangle == m_premier_triangle) {
            m_premier_triangle = triangle->suivant;

            if (m_premier_triangle) {
                m_premier_triangle->precedent = nullptr;
            }
        }

        if (triangle == m_dernier_triangle) {
            m_dernier_triangle = triangle->precedent;

            if (m_dernier_triangle) {
                m_dernier_triangle->precedent = nullptr;
            }
        }

        memoire::deloge("Triangle", triangle);
    }

    Triangle *premier_triangle()
    {
        return m_premier_triangle;
    }

    bool vide() const
    {
        return m_premier_triangle == nullptr;
    }
};

struct BoiteTriangle {
    float aire_minimum = std::numeric_limits<float>::max();
    float aire_maximum = 0.0f;  // = 2 * aire_minimum
    float aire_totale = 0.0f;
    float pad{};

    ListeTriangle triangles{};
};

static void ajoute_triangle_boite(BoiteTriangle *boite,
                                  dls::math::vec3f const &v0,
                                  dls::math::vec3f const &v1,
                                  dls::math::vec3f const &v2,
                                  long index)
{
    auto triangle = boite->triangles.ajoute(v0, v1, v2);
    triangle->index_orig = index;
    boite->aire_minimum = std::min(boite->aire_minimum, triangle->aire);
    boite->aire_maximum = 2 * boite->aire_minimum;
    boite->aire_totale += triangle->aire;
}

static BoiteTriangle *choisis_boite(BoiteTriangle boites[], GNA &gna)
{
    auto aire_totale_boites = 0.0f;

    auto nombre_boite_vide = 0;
    for (auto i = 0; i < NOMBRE_BOITE; ++i) {
        if (boites[i].triangles.vide()) {
            ++nombre_boite_vide;
            continue;
        }

        aire_totale_boites += boites[i].aire_totale;
    }

    if (nombre_boite_vide == NOMBRE_BOITE) {
        return nullptr;
    }

    auto prob_selection = gna.uniforme(0.0f, 1.0f) * aire_totale_boites;
    auto aire_courante = 0.0f;

    for (auto i = 0; i < NOMBRE_BOITE; i++) {
        if (boites[i].triangles.vide()) {
            continue;
        }

        if (prob_selection >= aire_courante &&
            prob_selection <= aire_courante + boites[i].aire_totale) {
            return &boites[i];
        }

        aire_courante += boites[i].aire_totale;
    }

    return nullptr;
}

static Triangle *choisis_triangle(BoiteTriangle *boite, GNA &gna)
{
#if 1
    static_cast<void>(gna);
    return boite->triangles.premier_triangle();
#else
    if (false) {  // cause un crash
        auto tri = boite->triangles.premier_triangle();
        boite->aire_totale = 0.0f;

        while (tri != nullptr) {
            boite->aire_totale += tri->aire;
            tri = tri->suivant;
        }
    }

    auto debut = compte_tick_ms();

    while (true) {
        auto tri = boite->triangles.premier_triangle();

        while (tri != nullptr) {
            auto const probabilite_triangle = tri->aire / boite->aire_totale;

            if (gna.uniforme(0.0f, 1.0f) <= probabilite_triangle) {
                return tri;
            }

            tri = tri->suivant;
        }

        /* Évite les boucles infinies. */
        if ((compte_tick_ms() - debut) > 1000) {
            break;
        }
    }

    return boite->triangles.premier_triangle();
#endif
}

struct DonneesAiresTriangles {
    dls::tableau<float> aire_par_triangle{};
    float aire_minimum = std::numeric_limits<float>::max();
    float aire_maximum = 0.0f;
    float aire_totale = 0.0f;
    dls::math::vec3f limites_min = dls::math::vec3f(std::numeric_limits<float>::max());
    dls::math::vec3f limites_max = dls::math::vec3f(-std::numeric_limits<float>::max());
};

static DonneesAiresTriangles calcule_donnees_aires(dls::tableau<Triangle> const &triangles)
{
    DonneesAiresTriangles resultat;
    resultat.aire_par_triangle.reserve(triangles.taille());

    /* Calcule les informations sur les aires. */
    for (auto const &triangle : triangles) {
        auto aire = static_cast<float>(calcule_aire(triangle));
        resultat.aire_minimum = std::min(resultat.aire_minimum, aire);
        resultat.aire_maximum = std::max(resultat.aire_maximum, aire);
        resultat.aire_totale += aire;
        resultat.aire_par_triangle.ajoute(aire);

        extrait_min_max(triangle.v0, resultat.limites_min, resultat.limites_max);
        extrait_min_max(triangle.v1, resultat.limites_min, resultat.limites_max);
        extrait_min_max(triangle.v2, resultat.limites_min, resultat.limites_max);
    }

    return resultat;
}

struct PointCree {
    dls::math::vec3f position;
    long index_triangle;
};

void distribue_particules_sur_surface(ParametreDistributionParticules const &params,
                                      Maillage const &surface,
                                      Maillage &points_resultants)
{
    auto nom_groupe = vers_std_string(params.ptr_nom_groupe_primitive,
                                      params.taille_nom_groupe_primitive);

    auto groupe_prim = static_cast<void *>(nullptr);

    if (params.utilise_groupe && !nom_groupe.empty()) {
        // À FAIRE : accès au groupe

        //        groupe_prim = corps_maillage->groupe_primitive(nom_groupe);

        //        if (groupe_prim == nullptr) {
        //            this->ajoute_avertissement("Aucun groupe de primitives nommé '",
        //                                       nom_groupe,
        //                                       "' trouvé sur le corps d'entrée !");

        //            return res_exec::ECHOUEE;
        //        }
    }

    /* Convertis le maillage en triangles. */
    auto triangles_entree = convertis_maillage_triangles(surface, groupe_prim);

    if (triangles_entree.est_vide()) {
        // this->ajoute_avertissement("Il n'y pas de polygones dans le corps d'entrée !");
        return;
    }

    /* Calcule les informations sur les aires. */
    auto const donnees_aires = calcule_donnees_aires(triangles_entree);

    /* Place les triangles dans les boites. */
    BoiteTriangle boites[NOMBRE_BOITE];

    for (auto const &triangle : triangles_entree) {
        auto aire = static_cast<float>(calcule_aire(triangle));

        auto const index_boite = static_cast<int>(std::log2(donnees_aires.aire_maximum / aire));

        if (index_boite < 0 || index_boite >= 64) {
            //            this->ajoute_avertissement("Erreur lors de la génération de l'index d'une
            //            boîte !",
            //                                       "\n   Index : ",
            //                                       index_boite,
            //                                       "\n   Aire triangle : ",
            //                                       aire,
            //                                       "\n   Aire totale : ",
            //                                       donnees_aires.aire_maximum);
            continue;
        }

        ajoute_triangle_boite(
            &boites[index_boite], triangle.v0, triangle.v1, triangle.v2, triangle.index_orig);
    }

    /* Ne considère que les triangles dont l'aire est supérieure à ce seuil. */
    auto const seuil_aire = donnees_aires.aire_minimum / 10000.0f;
    auto const distance = params.distance_minimale;

    /* Calcule le nombre maximum de point. */
    auto const aire_cercle = constantes<float>::PI * (distance * 0.5f) * (distance * 0.5f);
    auto const nombre_points = static_cast<long>((donnees_aires.aire_totale * DENSITE_CERCLE) /
                                                 aire_cercle);

    auto const graine = params.graine;

    auto gna = GNA(static_cast<unsigned long>(graine));

    auto grille_particule = GrilleParticules(dls::math::point3d(donnees_aires.limites_min),
                                             dls::math::point3d(donnees_aires.limites_max),
                                             distance);

    auto debut = compte_tick_ms();

    auto resultat = dls::tableau<PointCree>();
    resultat.reserve(nombre_points);

    /* Tant qu'il reste des triangles à remplir... */
    while (true) {
        /* Choisis une boîte avec une probabilité proportionnelle à l'aire
         * total des fragments de la boîte. */
        BoiteTriangle *boite = choisis_boite(boites, gna);

        /* Toutes les boites sont vides, arrêt de l'algorithme. */
        if (boite == nullptr) {
            break;
        }

        /* Sélectionne un triangle proportionellement à son aire. */
        auto triangle = choisis_triangle(boite, gna);

        /* Choisis un point aléatoire p sur le triangle en prenant une
         * coordonnée barycentrique aléatoire. */
        auto const v0 = triangle->v0;
        auto const v1 = triangle->v1;
        auto const v2 = triangle->v2;
        auto const e0 = v1 - v0;
        auto const e1 = v2 - v0;

        auto r = gna.uniforme(0.0f, 1.0f);
        auto s = gna.uniforme(0.0f, 1.0f);

        if (r + s >= 1.0f) {
            r = 1.0f - r;
            s = 1.0f - s;
        }

        auto point = v0 + r * e0 + s * e1;

        /* Vérifie que le point respecte la condition de distance minimal */
        auto ok = grille_particule.verifie_distance_minimal(point, distance);

        if (ok) {
            grille_particule.ajoute(point);
            resultat.ajoute({point, triangle->index_orig});
            debut = compte_tick_ms();
        }

        /* Vérifie si le triangle est complétement couvert par un point de
         * l'ensemble. */
        auto couvert = false;
        {
            /* Commençons d'abord par le point ajouté. */
            if (longueur(triangle->v0 - point) <= distance &&
                longueur(triangle->v1 - point) <= distance &&
                longueur(triangle->v2 - point) <= distance) {
                couvert = true;
            }
        }

        if (!couvert) {
            couvert = grille_particule.triangle_couvert(
                triangle->v0, triangle->v1, triangle->v2, distance);
        }

        if (couvert) {
            /* Si couvert, jète le triangle. */
            boite->aire_totale -= triangle->aire;
            boite->triangles.enleve(triangle);
        }
        else {
            /* Sinon, coupe le triangle en petit morceaux, et ajoute ceux
             * qui ne ne sont pas totalement couvert à la liste, sauf si son
             * aire est plus petite que le seuil d'acceptance. */

            /* On coupe le triangle en quatre en introduisant un point au
             * centre de chaque coté. */
            auto const v01 = (v0 + v1) * 0.5f;
            auto const v12 = (v1 + v2) * 0.5f;
            auto const v20 = (v2 + v0) * 0.5f;

            Triangle triangle_fils[4] = {
                Triangle{v0, v01, v20},
                Triangle{v01, v1, v12},
                Triangle{v12, v2, v20},
                Triangle{v20, v01, v12},
            };

            for (auto i = 0; i < 4; ++i) {
                auto const aire = calcule_aire(triangle_fils[i]);

                if (std::abs(aire - seuil_aire) <= std::numeric_limits<float>::epsilon()) {
                    boite->aire_totale -= aire;
                    continue;
                }

                couvert = grille_particule.triangle_couvert(
                    triangle_fils[i].v0, triangle_fils[i].v1, triangle_fils[i].v2, distance);

                if (couvert) {
                    continue;
                }

                auto const index_boite0 = static_cast<int>(
                    std::log2(donnees_aires.aire_maximum / aire));

                if (index_boite0 >= 0 && index_boite0 < 64) {
                    auto &b = boites[index_boite0];

                    auto t = b.triangles.ajoute(
                        triangle_fils[i].v0, triangle_fils[i].v1, triangle_fils[i].v2);
                    t->index_orig = triangle->index_orig;
                    b.aire_totale += aire;
                }
            }

            boite->triangles.enleve(triangle);
        }

        /* Évite les boucles infinies. */
        if ((compte_tick_ms() - debut) > 1000) {
            break;
        }
    }

    // À FAIRE : transfère les attributs.
    for (auto const &point : resultat) {
        points_resultants.ajouteUnPoint(point.position);
    }
}

}  // namespace geo
