/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "particules.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/outils/temps.hh"
#include "biblinternes/structures/grille_particules.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

#include "acceleration.hh"
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

    int64_t index_orig = 0;

    float aire = 0.0f;

    /* Fragmente le triangle en 4 sous-triangles en introduisant un point au centre de chaque côté.
     */
    std::array<Triangle, 4> fragmente() const
    {
        auto const v01 = (v0 + v1) * 0.5f;
        auto const v12 = (v1 + v2) * 0.5f;
        auto const v20 = (v2 + v0) * 0.5f;

        std::array<Triangle, 4> résultat = {
            Triangle{v0, v01, v20, index_orig},
            Triangle{v01, v1, v12, index_orig},
            Triangle{v12, v2, v20, index_orig},
            Triangle{v20, v01, v12, index_orig},
        };

        return résultat;
    }
};

static float calcule_aire(const Triangle &triangle)
{
    return calcule_aire(triangle.v0, triangle.v1, triangle.v2);
}

static dls::math::vec3f point_aleatoire(const Triangle &triangle, GNA &gna)
{
    auto const v0 = triangle.v0;
    auto const v1 = triangle.v1;
    auto const v2 = triangle.v2;
    auto const e0 = v1 - v0;
    auto const e1 = v2 - v0;

    auto r = gna.uniforme(0.0f, 1.0f);
    auto s = gna.uniforme(0.0f, 1.0f);

    if (r + s >= 1.0f) {
        r = 1.0f - r;
        s = 1.0f - s;
    }

    return v0 + r * e0 + s * e1;
}

static bool est_triangle_couvert(const Triangle &triangle,
                                 const dls::math::vec3f point_courant,
                                 const float distance,
                                 GrilleParticules &grille)
{
    /* Commençons d'abord par le point ajouté. */
    if (longueur(triangle.v0 - point_courant) <= distance &&
        longueur(triangle.v1 - point_courant) <= distance &&
        longueur(triangle.v2 - point_courant) <= distance) {
        return true;
    }

    return grille.triangle_couvert(triangle.v0, triangle.v1, triangle.v2, distance);
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

        for (int64_t j = 2; j < surface.nombreDeSommetsPolygone(i); ++j) {

            surface.pointPourSommetPolygones(i, j - 1, triangle.v1);
            surface.pointPourSommetPolygones(i, j, triangle.v2);
            triangle.index_orig = i;

            triangles[index_triangle++] = triangle;
        }
    }

    return triangles;
}

struct BoiteTriangle {
    float aire_minimum = std::numeric_limits<float>::max();
    float aire_maximum = 0.0f;  // = 2 * aire_minimum
    float aire_totale = 0.0f;
    float pad{};

    dls::tableau<Triangle> triangles{};

    void ajoute_triangle(Triangle const &triangle)
    {
        ajoute_sous_triangle(triangle);
        aire_minimum = std::min(aire_minimum, calcule_aire(triangle));
        aire_maximum = 2 * aire_minimum;
    }

    void ajoute_sous_triangle(Triangle const &triangle)
    {
        triangles.ajoute(triangle);
        aire_totale += calcule_aire(triangle);
    }

    void enleve_triangle(Triangle const *triangle)
    {
        auto index = std::distance(&triangles[0], const_cast<Triangle *>(triangle));
        std::swap(triangles[index], triangles.back());
        triangles.pop_back();
        aire_totale -= calcule_aire(*triangle);
    }
};

static void ajoute_triangle_boite(BoiteTriangle *boite,
                                  dls::math::vec3f const &v0,
                                  dls::math::vec3f const &v1,
                                  dls::math::vec3f const &v2,
                                  int64_t index)
{
    auto triangle = Triangle{v0, v1, v2, index};
    triangle.aire = calcule_aire(triangle);
    boite->ajoute_triangle(triangle);
}

class GestionnaireFragment {
    mutable BoiteTriangle boites[NOMBRE_BOITE] = {};
    float aire_maximum = 0.0f;

  public:
    explicit GestionnaireFragment(const float aire_max) : aire_maximum(aire_max)
    {
    }

    void ajoute_fragment_initial(Triangle const &triangle)
    {
        auto aire = static_cast<float>(calcule_aire(triangle));
        auto const index_boite = index_pour_boite(aire);

        if (index_boite < 0 || index_boite >= 64) {
            //            this->ajoute_avertissement("Erreur lors de la génération de l'index d'une
            //            boîte !",
            //                                       "\n   Index : ",
            //                                       index_boite,
            //                                       "\n   Aire triangle : ",
            //                                       aire,
            //                                       "\n   Aire totale : ",
            //                                       donnees_aires.aire_maximum);
            return;
        }

        ajoute_triangle_boite(
            &boites[index_boite], triangle.v0, triangle.v1, triangle.v2, triangle.index_orig);
    }

    void ajoute_fragment(Triangle const &triangle, const float aire)
    {
        auto const index_boite = index_pour_boite(aire);
        if (index_boite < 0 || index_boite >= 64) {
            return;
        }
        auto &b = boites[index_boite];
        b.ajoute_sous_triangle(triangle);
    }

    std::optional<Triangle> choisis_fragment(GNA &gna) const
    {
        BoiteTriangle *boite = choisis_boite(gna);
        if (boite == nullptr) {
            return {};
        }

        auto triangle = choisis_triangle(boite, gna);
        if (triangle == nullptr) {
            return {};
        }

        auto res = *triangle;

        /* Supprime directement le triangle de la boite, car :
         * - soit le triangle sera couvert
         * - soit le triangle sera fragmenté.
         * Dans les deux cas, il sera supprimé de la boite. */
        boite->enleve_triangle(triangle);

        return res;
    }

  private:
    int index_pour_boite(const float aire) const
    {
        return static_cast<int>(std::log2(aire_maximum / aire));
    }

    /* Choisis une boîte avec une probabilité proportionnelle à l'aire
     * total des fragments de la boîte. */
    BoiteTriangle *choisis_boite(GNA &gna) const
    {
        auto aire_totale_boites = 0.0f;

        auto nombre_boite_vide = 0;
        for (auto &boite : boites) {
            if (boite.triangles.est_vide()) {
                ++nombre_boite_vide;
                continue;
            }

            aire_totale_boites += boite.aire_totale;
        }

        if (nombre_boite_vide == NOMBRE_BOITE) {
            return nullptr;
        }

        auto prob_selection = gna.uniforme(0.0f, 1.0f) * aire_totale_boites;
        auto aire_courante = 0.0f;

        for (auto &boite : boites) {
            if (boite.triangles.est_vide()) {
                continue;
            }

            if (prob_selection >= aire_courante &&
                prob_selection <= aire_courante + boite.aire_totale) {
                return &boite;
            }

            aire_courante += boite.aire_totale;
        }

        return nullptr;
    }

    /* Sélectionne un triangle proportionellement à son aire. */
    Triangle *choisis_triangle(BoiteTriangle *boite, GNA &gna) const
    {
        if (boite->triangles.taille() == 1) {
            return &boite->triangles[0];
        }

        auto triangle_potentiel = gna.uniforme(0l, boite->triangles.taille() - 1);
        auto const prob_selection = gna.uniforme(0.0f, 1.0f);

        while (true) {
            auto triangle = &boite->triangles[triangle_potentiel];
            auto aire = calcule_aire(*triangle);
            auto const probabilite_triangle = aire / boite->aire_maximum;

            if (gna.uniforme(0.0f, 1.0f) < probabilite_triangle) {
                return triangle;
            }

            triangle_potentiel = gna.uniforme(0l, boite->triangles.taille() - 1);
        }

        return nullptr;
    }
};

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
    DonneesAiresTriangles résultat;
    résultat.aire_par_triangle.reserve(triangles.taille());

    /* Calcule les informations sur les aires. */
    for (auto const &triangle : triangles) {
        auto aire = static_cast<float>(calcule_aire(triangle));
        résultat.aire_minimum = std::min(résultat.aire_minimum, aire);
        résultat.aire_maximum = std::max(résultat.aire_maximum, aire);
        résultat.aire_totale += aire;
        résultat.aire_par_triangle.ajoute(aire);

        extrait_min_max(triangle.v0, résultat.limites_min, résultat.limites_max);
        extrait_min_max(triangle.v1, résultat.limites_min, résultat.limites_max);
        extrait_min_max(triangle.v2, résultat.limites_min, résultat.limites_max);
    }

    return résultat;
}

struct PointCree {
    dls::math::vec3f position;
    int64_t index_triangle;
    float rayon;
};

struct CouverturePonctuelle {
    float distance_minimale = 0.0f;
    int nombre_requis = 0;
};

static CouverturePonctuelle determine_couverture_ponctuelle(
    ParametreDistributionParticules const &params, float const aire_totale)
{
    CouverturePonctuelle résultat;

    switch (params.determination_quantite_points) {
        case DET_QT_PNT_PAR_DISTANCE:
        {
            auto const distance = params.distance_minimale;
            auto const aire_cercle = constantes<float>::PI * (distance * distance);
            auto const nombre_points = static_cast<int64_t>((aire_totale * DENSITE_CERCLE) /
                                                            aire_cercle);
            résultat.distance_minimale = distance;
            résultat.nombre_requis = nombre_points;
            break;
        }
        case DET_QT_PNT_PAR_NOMBRE_ABSOLU:
        {
            auto const nombre_points = params.nombre_absolu;
            résultat.distance_minimale = sqrt(((aire_totale * DENSITE_CERCLE) / (nombre_points)) /
                                              constantes<float>::PI);
            résultat.nombre_requis = nombre_points;
            break;
        }
    }

    return résultat;
}

class RayonnementUniforme {
    float rayon = 0.0f;

  public:
    RayonnementUniforme(float r) : rayon(r)
    {
    }

    float operator()()
    {
        return rayon;
    }
};

class RayonnementAleatoire {
    float min = 0.0f;
    float max = 0.0f;
    GNA &gna;

  public:
    RayonnementAleatoire(GNA &gna_, float min_, float max_) : min(min_), max(max_), gna(gna_)
    {
    }

    float operator()()
    {
        return gna.uniforme(min, max);
    }
};

template <typename Rayonnement>
static dls::tableau<PointCree> distribue_particules_sur_surface(
    GestionnaireFragment &gestionnaire_fragments,
    GrilleParticules &grille_particule,
    CouverturePonctuelle const &couverture,
    GNA &gna,
    float const seuil_aire,
    Rayonnement rayonnement)
{
    auto debut = compte_tick_ms();

    auto résultat = dls::tableau<PointCree>();
    résultat.reserve(couverture.nombre_requis);

    /* Tant qu'il reste des triangles à remplir, ou des points à distribuer. */
    auto points_restants = couverture.nombre_requis;
    while (true) {
        /* Sélectionne un triangle proportionellement à son aire. */
        auto triangle = gestionnaire_fragments.choisis_fragment(gna);
        if (!triangle.has_value()) {
            /* Plus aucun fragment. */
            break;
        }

        /* Choisis un point aléatoire sur le triangle en prenant une
         * coordonnée barycentrique aléatoire. */
        auto point = point_aleatoire(*triangle, gna);

        auto distance = rayonnement();

        /* Vérifie que le point respecte la condition de distance minimal */
        auto ok = grille_particule.verifie_distance_minimal(point, distance);

        if (ok) {
            grille_particule.ajoute(point);
            /* Le rayon est la moitié de la distance entre les points. */
            résultat.ajoute({point, triangle->index_orig, distance * 0.5f});
            debut = compte_tick_ms();
            points_restants--;
        }

        /* Vérifie si le triangle est complétement couvert par un point de
         * l'ensemble. */
        if (est_triangle_couvert(*triangle, point, distance, grille_particule)) {
            continue;
        }

        /* Sinon, coupe le triangle en petit morceaux, et ajoute ceux
         * qui ne ne sont pas totalement couvert à la liste, sauf si son
         * aire est plus petite que le seuil d'acceptance. */
        auto const triangles_fils = triangle->fragmente();

        for (auto triangle_fils : triangles_fils) {
            auto const aire = calcule_aire(triangle_fils);

            if (std::abs(aire - seuil_aire) <= std::numeric_limits<float>::epsilon()) {
                continue;
            }

            if (est_triangle_couvert(triangle_fils, point, distance, grille_particule)) {
                continue;
            }

            gestionnaire_fragments.ajoute_fragment(triangle_fils, aire);
        }

        /* Évite les boucles infinies. */
        if ((compte_tick_ms() - debut) > 1000) {
            break;
        }
    }

    return résultat;
}

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
    GestionnaireFragment gestionnaire_fragments(donnees_aires.aire_maximum);

    for (auto const &triangle : triangles_entree) {
        gestionnaire_fragments.ajoute_fragment_initial(triangle);
    }

    /* Ne considère que les triangles dont l'aire est supérieure à ce seuil. */
    auto const seuil_aire = donnees_aires.aire_minimum / 10000.0f;

    /* Calcule la couverture ponctuelle. */
    auto const couverture = determine_couverture_ponctuelle(params, donnees_aires.aire_totale);
    auto const distance = couverture.distance_minimale;

    auto const graine = params.graine;

    auto gna = GNA(static_cast<uint64_t>(graine));

    auto grille_particule = GrilleParticules(dls::math::point3d(donnees_aires.limites_min),
                                             dls::math::point3d(donnees_aires.limites_max),
                                             distance);

    dls::tableau<PointCree> résultat;

    if (params.type_rayonnement == TypeRayonnementPoint::RAYONNEMENT_UNIFORME) {
        résultat = distribue_particules_sur_surface(
            gestionnaire_fragments,
            grille_particule,
            couverture,
            gna,
            seuil_aire,
            RayonnementUniforme(couverture.distance_minimale));
    }
    else {
        résultat = distribue_particules_sur_surface(
            gestionnaire_fragments,
            grille_particule,
            couverture,
            gna,
            seuil_aire,
            RayonnementAleatoire(gna, couverture.distance_minimale, params.distance_maximale));
    }

    // À FAIRE : transfère les attributs.
    for (auto const &point : résultat) {
        points_resultants.ajouteUnPoint(point.position);
    }

    auto const nom_attr_rayon = vers_std_string(params.ptr_nom_rayon, params.taille_nom_rayon);
    if (params.exporte_rayon && nom_attr_rayon != "") {
        int index_point = 0;
        AttributReel attr_rayon = points_resultants.ajouteAttributPoint<R32>(nom_attr_rayon);

        if (attr_rayon) {
            for (auto const &point : résultat) {
                attr_rayon.ecris_reel(index_point++, point.rayon);
            }
        }
    }
}

/* **************************************************************** */

namespace bridson {

// A configuration structure to customise the poisson_disc_distribution
// algorithm below.
//
//   width, height - Defines the range of x as (0, width] and the range
//                   of y as (0, height].
//
//   min_distance  - The smallest distance allowed between two points. Also,
//                   points will never be further apart than twice this
//                   distance.
//
//   max_attempts  - The algorithm stochastically attempts to place a new point
//                   around a current point. This number limits the number of
//                   attempts per point. A lower number will speed up the
//                   algorithm but at some cost, possibly significant, to the
//                   result's aesthetics.
//
//   start         - An optional parameter. If set to anything other than
//                   point's default values (infinity, infinity) the algorithm
//                   will start from this point. Otherwise a point is chosen
//                   randomly. Expected to be within the region defined by
//                   width and height.
struct config {
    float largeur = 1.0f;
    float hauteur = 1.0f;
    float distance_minimale = 0.05f;
    int maximum_de_tentatives = 30;
    dls::math::point2f origine{constantes<float>::INFINITE};
};

// This implements the algorithm described in 'Fast Poisson Disk Sampling in
// Arbitrary Dimensions' by Robert Bridson. This produces a random set of
// points such that no two points are closer than conf.min_distance apart or
// further apart than twice that distance.
//
// Parameters
//
//   conf    - The configuration, as detailed above.
//
//   random  - A callback of the form float(float limit) that returns a random
//             value ranging from 0 (inclusive) to limit (exclusive).
//
//   in_area - A callback of the form bool(point) that returns whether a point
//             is within a valid area. This can be used to create shapes other
//             than rectangles. Points can't be outside of the defined limits of
//             the width and height specified. See the notes section for more.
//
//   output  - A callback of the form void(point). All points that are part of
//             the final Poisson disc distribution are passed here.
//
// Notes
//
//   The time complexity is O(n) where n is the number of points.
//
//   The in_area callback must prevent points from leaving the region defined by
//   width and height (i.e., 0 <= x < width and 0 <= y < height). If this is
//   not done invalid memory accesses will occur and most likely a segmentation
//   fault.
template <typename T, typename T2, typename T3>
void poisson_disc_distribution(config conf, T &&random, T2 &&in_area, T3 &&output)
{
    auto cell_size = conf.distance_minimale / std::sqrt(2.0f);
    auto grid_width = static_cast<int>(std::ceil(conf.largeur / cell_size));
    auto grid_height = static_cast<int>(std::ceil(conf.hauteur / cell_size));

    dls::tableau<dls::math::point2f> grid(grid_width * grid_height,
                                          dls::math::point2f(constantes<float>::INFINITE));
    dls::pile<dls::math::point2f> process;

    auto squared_distance = [](const dls::math::point2f &a, const dls::math::point2f &b) {
        auto delta_x = a.x - b.x;
        auto delta_y = a.y - b.y;

        return delta_x * delta_x + delta_y * delta_y;
    };

    auto point_around = [&conf, &random](dls::math::point2f p) {
        auto radius = random(conf.distance_minimale) + conf.distance_minimale;
        auto angle = random(constantes<float>::TAU);

        p.x += std::cos(angle) * radius;
        p.y += std::sin(angle) * radius;

        return p;
    };

    auto set = [cell_size, grid_width, &grid](const dls::math::point2f &p) {
        auto x = static_cast<int>(p.x / cell_size);
        auto y = static_cast<int>(p.y / cell_size);
        grid[y * grid_width + x] = p;
    };

    auto add = [&process, &output, &set](const dls::math::point2f &p) {
        process.empile(p);
        output(p);
        set(p);
    };

    auto point_too_close = [&](const dls::math::point2f &p) {
        auto x_index = static_cast<int>(std::floor(p.x / cell_size));
        auto y_index = static_cast<int>(std::floor(p.y / cell_size));

        if (!dls::math::sont_environ_egaux(grid[y_index * grid_width + x_index].x,
                                           constantes<float>::INFINITE)) {
            return true;
        }

        auto min_dist_squared = conf.distance_minimale * conf.distance_minimale;
        auto min_x = std::max(x_index - 2, 0);
        auto min_y = std::max(y_index - 2, 0);
        auto max_x = std::min(x_index + 2, grid_width - 1);
        auto max_y = std::min(y_index + 2, grid_height - 1);

        for (auto y = min_y; y <= max_y; ++y) {
            for (auto x = min_x; x <= max_x; ++x) {
                auto point = grid[y * grid_width + x];
                auto exists = !dls::math::sont_environ_egaux(point.x, constantes<float>::INFINITE);

                if (exists && squared_distance(p, point) < min_dist_squared) {
                    return true;
                }
            }
        }

        return false;
    };

    if (dls::math::sont_environ_egaux(conf.origine.x, constantes<float>::INFINITE)) {
        do {
            conf.origine.x = random(conf.largeur);
            conf.origine.y = random(conf.hauteur);
        } while (!in_area(conf.origine));
    }

    add(conf.origine);

    while (!process.est_vide()) {
        auto point = process.depile();

        for (int i = 0; i != conf.maximum_de_tentatives; ++i) {
            auto p = point_around(point);

            if (in_area(p) && !point_too_close(p)) {
                add(p);
            }
        }
    }
}

}  // namespace bridson

void distribue_poisson_2d(ParametresDistributionPoisson2D const &params,
                          Maillage &points_resultants)
{
    if (params.longueur == 0.0f || params.largeur == 0.0f) {
        return;
    }

    bridson::config conf;
    conf.distance_minimale = params.distance_minimale;
    conf.largeur = params.largeur;
    conf.hauteur = params.longueur;
    conf.origine.x = params.origine_x;
    conf.origine.y = params.origine_y;

    auto gna = GNA(params.graine);

    auto vertices = dls::tableau<dls::math::vec2f>();

    bridson::poisson_disc_distribution(
        conf,
        // random
        [&gna](float range) { return gna.uniforme(0.0f, range); },
        // in_area
        [&](dls::math::point2f const &p) {
            return params.peut_ajouter_point(params.donnees_utilisateur, p.x, p.y);
        },
        // output
        [&vertices](dls::math::point2f const &p) { vertices.ajoute(dls::math::vec2f(p.x, p.y)); });

    points_resultants.reserveNombreDePoints(vertices.taille());
    for (auto &point : vertices) {
        points_resultants.ajouteUnPoint(point.x, point.y, 0.0f);
    }
}

/* ************************************************************************** */

/**
 * Implémentation de l'algorithme de génération de maillage alpha de
 * "Enhancing Particle Methods for Fluid Simulation in Computer Graphics",
 * Hagit Schechter, 2013
 * https://www.cs.ubc.ca/~rbridson/docs/schechter_phd.pdf
 *
 * Voir également :
 * - https://lidarwidgets.com/samples/bpa_tvcg.pdf (Ball Pivoting Algorithm)
 */

static bool construit_sphere(dls::math::vec3f const &x0,
                             dls::math::vec3f const &x1,
                             dls::math::vec3f const &x2,
                             float const rayon,
                             dls::math::vec3f &centre)
{
    auto const x0x1 = x0 - x1;
    auto const lx0x1 = longueur(x0x1);

    auto const x1x2 = x1 - x2;
    auto const lx1x2 = longueur(x1x2);

    auto const x2x0 = x2 - x0;
    auto const lx2x0 = longueur(x2x0);

    auto n = produit_croix(x0x1, x1x2);
    auto ln = longueur(n);

    auto radius_x = (lx0x1 * lx1x2 * lx2x0) / (2.0f * ln);

    if (radius_x > rayon) {
        return false;
    }

    auto const abs_n_sqr = (ln * ln);
    auto const inv_abs_n_sqr2 = 1.0f / (2.0f * abs_n_sqr);

    auto alpha = (longueur_carree(x1x2) * produit_scalaire(x0x1, x0 - x2)) * inv_abs_n_sqr2;
    auto beta = (longueur_carree(x0 - x2) * produit_scalaire(x1 - x0, x1x2)) * inv_abs_n_sqr2;
    auto gamma = (longueur_carree(x0x1) * produit_scalaire(x2x0, x2 - x1)) * inv_abs_n_sqr2;

    auto l = alpha * x0 + beta * x1 + gamma * x2;

    /* NOTE : selon le papier, c'est censé être
     * (radius_x * radius_x - radius * radius)
     * mais cela donne un nombre négatif, résultant en un NaN... */
    auto t = std::sqrt(abs(radius_x * radius_x - rayon * rayon));

    centre = l + t * n;

    if (est_nan(centre)) {
        return false;
    }

    return true;
}

static dls::tableau<int> trouve_points_voisins(arbre_3df const &points,
                                               dls::math::vec3f const &point,
                                               const int index_point,
                                               const float radius)
{
    dls::tableau<int> résultat;
    points.cherche_points(
        point, radius, [&](int64_t index, dls::math::vec3f const &, float, float &) {
            if (index == index_point) {
                return;
            }
            résultat.ajoute(index);
        });
    return résultat;
}

static bool tous_les_autres_points_sont_exclus(Maillage const &points,
                                               dls::tableau<int> const &N1,
                                               int j,
                                               int k,
                                               dls::math::vec3f center,
                                               float rayon)
{
    for (auto i(0); i < N1.taille(); ++i) {
        if (i == j || i == k) {
            continue;
        }

        if (longueur(points.pointPourIndex(N1[i]) - center) <= rayon) {
            return false;
        }
    }

    return true;
}

static void construit_triangle(Maillage const &points,
                               Maillage &maillage_résultat,
                               int i,
                               float const radius,
                               dls::tableau<int> const &N1,
                               dls::math::vec3f const &pi)
{
    for (auto j = 0; j < N1.taille() - 1; ++j) {
        auto const pj = points.pointPourIndex(N1[j]);

        for (auto k = j + 1; k < N1.taille(); ++k) {
            auto const pk = points.pointPourIndex(N1[k]);

            dls::math::vec3f center;
            if (!construit_sphere(pi, pj, pk, radius, center)) {
                continue;
            }

            /* Vérifie qu'il n'est pas de points à l'intérieur de la sphère. */
            if (!tous_les_autres_points_sont_exclus(points, N1, j, k, center, radius)) {
                continue;
            }

            int poly[3] = {i, N1[j], N1[k]};
            maillage_résultat.ajouteUnPolygone(poly, 3);
            /* Ne retournons de suite, il peut y avoir d'autres triangles. */
        }
    }
}

void construit_maillage_alpha(Maillage const &points,
                              const float rayon,
                              Maillage &maillage_résultat)
{
    for (auto i = 0; i < points.nombreDePoints(); ++i) {
        maillage_résultat.ajouteUnPoint(points.pointPourIndex(i));
    }

    auto arbre = arbre_3df();
    arbre.construit_avec_fonction(points.nombreDePoints(), [&](int64_t i) -> dls::math::vec3f {
        return points.pointPourIndex(i);
    });

    for (auto i = 0; i < points.nombreDePoints(); ++i) {
        auto point = points.pointPourIndex(i);
        auto N1 = trouve_points_voisins(arbre, point, i, 2.0f * rayon);
        construit_triangle(points, maillage_résultat, i, rayon, N1, point);
    }
}

}  // namespace geo
