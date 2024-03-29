﻿/*
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

#include "operatrices_vetements.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/iteration_corps.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/* Un simple solveur de dynamique de vêtement utilisant des dynamiques basées
 * sur les positions tiré des notes du cours de SIGGRAPH "Realtime Physics"
 * http://www.matthiasmueller.info/realtimephysics/coursenotes.pdf
 *
 * Tiré de "OpenCloth", voir discussion sur
 * http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=6&t=7013&p=24420#p24420
 */

/* ************************************************************************** */

#define EPSILON 0.0000001f

struct DistanceConstraint {
    /* index des points */
    long p1, p2;
    float longueur_repos;
    /* coefficient d'étirement */
    float k;
    /* 1 - coefficient d'étirement */
    float k_prime;

    float pad;
};

struct BendingConstraint {
    /* index des points */
    long p1, p2, p3;
    float longueur_repos;
    /* poids */
    float w;
    /* coefficient de courbure */
    float k;
    /* 1 - coefficient de courbure */
    float k_prime;
};

static auto ajoute_contrainte_distance(
    AccesseusePointEcriture &X, long a, long b, float k, int solver_iterations)
{
    DistanceConstraint c;
    c.p1 = a;
    c.p2 = b;
    c.k = k;
    c.k_prime = 1.0f - std::pow((1.0f - c.k), 1.0f / static_cast<float>(solver_iterations));

    if (c.k_prime > 1.0f) {
        c.k_prime = 1.0f;
    }

    auto const deltaP = X.point_local(c.p1) - X.point_local(c.p2);
    c.longueur_repos = longueur(deltaP);

    return c;
}

static auto ajoute_contrainte_courbure(AccesseusePointEcriture &X,
                                       Attribut *W,
                                       long pa,
                                       long pb,
                                       long pc,
                                       float k,
                                       int solver_iterations)
{
    BendingConstraint c;
    c.p1 = pa;
    c.p2 = pb;
    c.p3 = pc;

    c.w = W->r32(pa)[0] + W->r32(pb)[0] + 2.0f * W->r32(pc)[0];
    auto const centre = 0.3333f * (X.point_local(pa) + X.point_local(pb) + X.point_local(pc));
    c.longueur_repos = longueur(X.point_local(pc) - centre);
    c.k = k;
    c.k_prime = 1.0f - std::pow((1.0f - c.k), 1.0f / static_cast<float>(solver_iterations));

    if (c.k_prime > 1.0f) {
        c.k_prime = 1.0f;
    }

    return c;
}

static void calcul_forces(Attribut *F, Attribut *W, dls::math::vec3f const &gravity)
{
    for (auto i = 0; i < F->taille(); i++) {
        assigne(F->r32(i), dls::math::vec3f(0.0f));

        if (W->r32(i)[0] > 0.0f) {
            assigne(F->r32(i), gravity);
        }
    }
}

static void integre_explicitement_avec_attenuation(float deltaTime,
                                                   Attribut *tmp_X,
                                                   Attribut *V,
                                                   Attribut *F,
                                                   AccesseusePointEcriture &X,
                                                   Attribut *Ri,
                                                   Attribut *W,
                                                   float mass,
                                                   float kDamp,
                                                   float global_dampening)
{
    auto v = dls::math::vec3f();
    auto f = dls::math::vec3f();
    auto ri = dls::math::vec3f();
    auto Xcm = dls::math::vec3f(0.0f);
    auto Vcm = dls::math::vec3f(0.0f);
    auto sumM = 0.0f;

    for (auto i = 0; i < X.taille(); ++i) {
        extrait(V->r32(i), v);
        extrait(F->r32(i), f);
        auto Vi = (v * global_dampening) + (f * deltaTime) * W->r32(i)[0];
        assigne(V->r32(i), Vi);

        /* calcul la position et la vélocité du centre de masse pour l'atténuation */
        Xcm += (X.point_local(i) * mass);
        Vcm += (Vi * mass);
        sumM += mass;
    }

    Xcm /= sumM;
    Vcm /= sumM;

    auto I = dls::math::mat3x3f{};
    auto L = dls::math::vec3f(0.0f);

    /* vélocité angulaire */
    auto w = dls::math::vec3f(0.0f);

    for (auto i = 0; i < X.taille(); ++i) {
        auto Ri_i = (X.point_local(i) - Xcm);
        assigne(Ri->r32(i), Ri_i);

        extrait(V->r32(i), v);
        L += produit_croix(Ri_i, mass * v);

        /* voir http://www.sccg.sk/~onderik/phd/ca2010/ca10_lesson11.pdf */
        auto tmp = dls::math::mat3x3f(
            0.0f, -Ri_i.z, Ri_i.y, Ri_i.z, 0.0f, -Ri_i.x, -Ri_i.y, Ri_i.x, 0.0f);

        tmp *= transpose(tmp);

        for (auto j = 0ul; j < 3ul; ++j) {
            for (auto k = 0ul; k < 3ul; ++k) {
                tmp[j][k] *= mass;
            }
        }

        I += tmp;
    }

    w = inverse(I) * L;

    /* applique l'atténuation du centre de masse */
    for (auto i = 0; i < X.taille(); ++i) {
        extrait(V->r32(i), v);
        extrait(Ri->r32(i), ri);
        auto delVi = Vcm + produit_croix(w, ri) - v;
        assigne(V->r32(i), v + kDamp * delVi);
    }

    /* calcul position prédite */
    for (auto i = 0; i < X.taille(); ++i) {
        if (W->r32(i)[0] <= 0.0f) {
            assigne(tmp_X->r32(i), X.point_local(i));  // fixed points
        }
        else {
            extrait(V->r32(i), v);
            assigne(tmp_X->r32(i), X.point_local(i) + v * deltaTime);
        }
    }
}

static void integre(float deltaTime, Attribut *V, AccesseusePointEcriture &X, Attribut *tmp_X)
{
    auto const inv_dt = 1.0f / deltaTime;
    auto tmp_x = dls::math::vec3f();

    for (auto i = 0; i < X.taille(); i++) {
        extrait(tmp_X->r32(i), tmp_x);
        assigne(V->r32(i), (tmp_x - X.point_local(i)) * inv_dt);
        X.point(i, tmp_x);
    }
}

static void ajourne_contraintes_distance(int i,
                                         dls::tableau<DistanceConstraint> const &d_constraints,
                                         Attribut *tmp_X,
                                         Attribut *W)
{
    auto xp1 = dls::math::vec3f();
    auto xp2 = dls::math::vec3f();

    auto const &c = d_constraints[i];
    extrait(tmp_X->r32(c.p1), xp1);
    extrait(tmp_X->r32(c.p2), xp2);

    auto const dir = xp1 - xp2;
    auto const len = longueur(dir);

    if (len <= EPSILON) {
        return;
    }

    auto const w1 = W->r32(c.p1)[0];
    auto const w2 = W->r32(c.p2)[0];
    auto const invMass = w1 + w2;

    if (invMass <= EPSILON) {
        return;
    }

    auto const dP = (1.0f / invMass) * (len - c.longueur_repos) * (dir / len) * c.k_prime;

    if (w1 > 0.0f) {
        assigne(tmp_X->r32(c.p1), xp1 - dP * w1);
    }

    if (w2 > 0.0f) {
        assigne(tmp_X->r32(c.p2), xp2 + dP * w2);
    }
}

static void ajourne_contraintes_courbures(int index,
                                          dls::tableau<BendingConstraint> const &b_constraints,
                                          Attribut *tmp_X,
                                          Attribut *W,
                                          float global_dampening)
{
    auto const &c = b_constraints[index];

    /* Utilisation de l'algorithme tiré du papier
     * 'A Triangle Bending Constraint Model for Position-Based Dynamics'
     * http://image.diku.dk/kenny/download/kelager.niebe.ea10.pdf
     */

    auto xp1 = dls::math::vec3f();
    auto xp2 = dls::math::vec3f();
    auto xp3 = dls::math::vec3f();
    extrait(tmp_X->r32(c.p1), xp1);
    extrait(tmp_X->r32(c.p2), xp2);
    extrait(tmp_X->r32(c.p3), xp3);

    auto const global_k = global_dampening * 0.01f;
    auto const centre = 0.3333f * (xp1 + xp2 + xp3);
    auto const dir_centre = xp3 - centre;
    auto const dist_centre = longueur(dir_centre);

    auto const diff = 1.0f - ((global_k + c.longueur_repos) / dist_centre);
    auto const dir_force = dir_centre * diff;
    auto const fa = c.k_prime * ((2.0f * W->r32(c.p1)[0]) / c.w) * dir_force;
    auto const fb = c.k_prime * ((2.0f * W->r32(c.p2)[0]) / c.w) * dir_force;
    auto const fc = -c.k_prime * ((4.0f * W->r32(c.p3)[0]) / c.w) * dir_force;

    if (W->r32(c.p1)[0] > 0.0f) {
        assigne(tmp_X->r32(c.p1), xp1 + fa);
    }

    if (W->r32(c.p2)[0] > 0.0f) {
        assigne(tmp_X->r32(c.p2), xp2 + fb);
    }

    if (W->r32(c.p3)[0] > 0.0f) {
        assigne(tmp_X->r32(c.p3), xp3 + fc);
    }
}

static void ajourne_contraintes_internes(Attribut *tmp_X,
                                         dls::tableau<DistanceConstraint> const &d_constraints,
                                         dls::tableau<BendingConstraint> const &b_constraints,
                                         Attribut *W,
                                         int solver_iterations,
                                         float global_dampening)
{
    for (auto si = 0; si < solver_iterations; ++si) {
        for (auto i = 0; i < d_constraints.taille(); i++) {
            ajourne_contraintes_distance(i, d_constraints, tmp_X, W);
        }

        for (auto i = 0; i < b_constraints.taille(); i++) {
            ajourne_contraintes_courbures(i, b_constraints, tmp_X, W, global_dampening);
        }
    }
}

/* ************************************************************************** */

struct ContrainteDistance {
    float longueur_repos;
    float pad;
    long v0;
    long v1;
};

struct DonneesSimVerlet {
    dls::tableau<ContrainteDistance> contrainte_distance;
    float drag;
    int repetitions;
    float dt;
    float pad;
};

static void integre_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
    auto attr_P = corps.attribut("P_prev");
    auto attr_F = corps.attribut("F");
    auto points = corps.points_pour_ecriture();

    boucle_parallele(tbb::blocked_range<long>(0, points.taille()),
                     [&](tbb::blocked_range<long> const &plage) {
                         for (auto i = plage.begin(); i < plage.end(); ++i) {
                             auto pos_cour = points.point_local(i);
                             auto pos_prev = dls::math::vec3f();
                             extrait(attr_P->r32(i), pos_prev);
                             auto force = dls::math::vec3f();
                             extrait(attr_F->r32(i), force);
                             auto drag = 1.0f - donnees_sim.drag;

                             /* integration */
                             auto vel = (pos_cour - pos_prev) + force * donnees_sim.dt;
                             auto pos_nouv = (pos_cour + vel * donnees_sim.dt * drag);

                             /* ajourne données solveur */
                             assigne(attr_P->r32(i), pos_cour);
                             points.point(i, pos_nouv);
                         }
                     });
}

static void contraintes_distance_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
    auto points = corps.points_pour_ecriture();

    for (auto const &contrainte : donnees_sim.contrainte_distance) {
        auto vec1 = points.point_local(contrainte.v0);
        auto vec2 = points.point_local(contrainte.v1);

        /* calcul nouvelles positions */
        auto const delta = vec2 - vec1;
        auto const longueur_delta = longueur(delta);
        auto const difference = (longueur_delta - contrainte.longueur_repos) / longueur_delta;
        vec1 = vec1 + delta * 0.5f * difference;
        vec2 = vec2 - delta * 0.5f * difference;

        /* ajourne positions */
        points.point(contrainte.v0, vec1);
        points.point(contrainte.v1, vec2);
    }
}

static void contraintes_position_verlet(Corps &corps, DonneesSimVerlet const & /*donnees_sim*/)
{
    auto attr_P = corps.attribut("P_prev");
    auto attr_W = corps.attribut("W");
    auto points = corps.points_pour_ecriture();

    boucle_parallele(tbb::blocked_range<long>(0, points.taille()),
                     [&](tbb::blocked_range<long> const &plage) {
                         for (auto i = plage.begin(); i < plage.end(); ++i) {
                             if (attr_W->r32(i)[0] <= 0.0f) {
                                 auto p = dls::math::vec3f();
                                 extrait(attr_P->r32(i), p);
                                 points.point(i, p);
                             }
                         }
                     });
}

static void applique_contraintes_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
    /* À FAIRE contraintes collision */
    contraintes_distance_verlet(corps, donnees_sim);
    contraintes_position_verlet(corps, donnees_sim);
}

/* ************************************************************************** */

static dls::ensemble<std::pair<long, long>> calcul_cote_unique(Corps &corps)
{
    dls::ensemble<std::pair<long, long>> ensemble_cote;

    pour_chaque_polygone(corps, [&](Corps const &, Polygone *poly) {
        for (auto j = 1; j < poly->nombre_sommets(); ++j) {
            auto j0 = poly->index_point(j - 1);
            auto j1 = poly->index_point(j);

            /* Ordonne les index pour ne pas compter les cotés allant
             * dans le sens opposé : (0, 1) = (1, 0). */
            ensemble_cote.insere(std::make_pair(std::min(j0, j1), std::max(j0, j1)));
        }

        /* diagonales, À FAIRE : + de 4 sommets */
        if (poly->nombre_sommets() == 4) {
            auto j0 = poly->index_point(0);
            auto j1 = poly->index_point(2);

            ensemble_cote.insere(std::make_pair(j0, j1));

            j0 = poly->index_point(1);
            j1 = poly->index_point(3);

            ensemble_cote.insere(std::make_pair(j0, j1));
        }
    });

    return ensemble_cote;
}

class OperatriceSimVetement final : public OperatriceCorps {
    dls::tableau<DistanceConstraint> d_constraints{};

    dls::tableau<BendingConstraint> b_constraints{};

    DonneesSimVerlet m_donnees_verlet{};

  public:
    static constexpr auto NOM = "Simulation Vêtement";
    static constexpr auto AIDE =
        "Simule un vêtement selon l'algorithme de Dynamiques Basées Point.";

    OperatriceSimVetement(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_simulation_vetement.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        if (!valide_corps_entree(*this, &m_corps, true, true)) {
            return res_exec::ECHOUEE;
        }

        auto integration = evalue_enum("intégration");

        if (integration == "verlet") {
            simule_verlet(contexte.temps_courant);
        }
        else {
            simule_dbp(contexte.temps_courant);
        }

        return res_exec::REUSSIE;
    }

    res_exec simule_verlet(int temps)
    {
        auto points_entree = m_corps.points_pour_lecture();

        auto const gravity = evalue_vecteur("gravité", temps);
        auto const mass = evalue_decimal("masse") / static_cast<float>(points_entree.taille());

        auto W = m_corps.ajoute_attribut("W", type_attribut::R32, 1, portee_attr::POINT);
        auto F = m_corps.ajoute_attribut("F", type_attribut::R32, 3, portee_attr::POINT);
        auto P = m_corps.ajoute_attribut("P_prev", type_attribut::R32, 3, portee_attr::POINT);

        /* À FAIRE : réinitialisation */
        if (temps == 1) {
            for (auto i = 0; i < points_entree.taille(); ++i) {
                assigne(P->r32(i), points_entree.point_local(i));
            }
        }

        for (auto i = 0; i < points_entree.taille(); ++i) {
            assigne(W->r32(i), 1.0f / mass);
        }

        /* points fixes, À FAIRE : groupe ou attribut. */
        for (auto i = 0; i < 20; ++i) {
            assigne(W->r32(i), 0.0f);
        }

        m_donnees_verlet.drag = evalue_decimal("atténuation", temps);
        m_donnees_verlet.repetitions = evalue_entier("itérations");
        m_donnees_verlet.dt = evalue_decimal("dt", temps);

        if (m_donnees_verlet.contrainte_distance.est_vide()) {
            auto ensemble_cote = calcul_cote_unique(m_corps);
            m_donnees_verlet.contrainte_distance.reserve(ensemble_cote.taille());

            for (auto &cote : ensemble_cote) {
                auto c = ContrainteDistance{};
                c.v0 = cote.first;
                c.v1 = cote.second;

                c.longueur_repos = longueur(points_entree.point_local(c.v0) -
                                            points_entree.point_local(c.v1));

                m_donnees_verlet.contrainte_distance.ajoute(c);
            }
        }

        /* lance la simulation */
        calcul_forces(F, W, gravity);

        integre_verlet(m_corps, m_donnees_verlet);

        for (int index = 0; index < m_donnees_verlet.repetitions; index++) {
            applique_contraintes_verlet(m_corps, m_donnees_verlet);
        }

        return res_exec::REUSSIE;
    }

    res_exec simule_dbp(int temps)
    {
        auto points_entree = m_corps.points_pour_ecriture();
        auto prims_entree = m_corps.prims();

        auto total_points = static_cast<size_t>(points_entree.taille());

        dls::tableau<unsigned short> indices;
        dls::tableau<float> phi0;  // initial dihedral angle between adjacent triangles

        auto X = points_entree;
        auto tmp_X = m_corps.ajoute_attribut("tmp_X", type_attribut::R32, 3, portee_attr::POINT);
        auto V = m_corps.ajoute_attribut("V", type_attribut::R32, 3, portee_attr::POINT);
        auto F = m_corps.ajoute_attribut("F", type_attribut::R32, 3, portee_attr::POINT);
        auto W = m_corps.ajoute_attribut("W", type_attribut::R32, 1, portee_attr::POINT);
        auto Ri = m_corps.ajoute_attribut("Ri", type_attribut::R32, 3, portee_attr::POINT);

        /* paramètres */
        auto const attenuation_globale = evalue_decimal("atténuation_globale", temps);
        auto const dt = evalue_decimal("dt");
        auto const iterations = evalue_entier("itérations");
        auto const courbe = evalue_decimal("courbe", temps);
        auto const etirement = evalue_decimal("étirement", temps);
        auto const attenuation = evalue_decimal("atténuation", temps);
        auto const gravity = evalue_vecteur("gravité", temps) * 0.001f;
        auto const mass = evalue_decimal("masse") / static_cast<float>(total_points);

        /* prépare données */

        for (auto i = 0; i < points_entree.taille(); ++i) {
            assigne(W->r32(i), 1.0f / mass);
        }

        /* points fixes, À FAIRE : groupe ou attribut. */
        for (auto i = 0; i < 20; ++i) {
            assigne(W->r32(i), 0.0f);
        }

        if (d_constraints.est_vide()) {
            auto ensemble_cote = calcul_cote_unique(m_corps);
            d_constraints.reserve(ensemble_cote.taille());

            for (auto &cote : ensemble_cote) {
                auto c = ajoute_contrainte_distance(
                    X, cote.first, cote.second, etirement, iterations);

                d_constraints.ajoute(c);
            }
        }

        if (b_constraints.est_vide()) {
            b_constraints.reserve(prims_entree->taille());

            pour_chaque_polygone(m_corps, [&](Corps const &, Polygone *poly) {
                for (auto j = 2; j < poly->nombre_sommets(); ++j) {
                    auto c = ajoute_contrainte_courbure(X,
                                                        W,
                                                        poly->index_point(0),
                                                        poly->index_point(j - 1),
                                                        poly->index_point(j),
                                                        courbe,
                                                        iterations);

                    b_constraints.ajoute(c);
                }
            });
        }

        if (d_constraints.est_vide() || b_constraints.est_vide()) {
            this->ajoute_avertissement(
                "Aucune contrainte n'a pu être ajoutée, aucun polygone trouvé !");
            return res_exec::ECHOUEE;
        }

        /* lance simulation */

        calcul_forces(F, W, gravity);

        integre_explicitement_avec_attenuation(
            dt, tmp_X, V, F, X, Ri, W, mass, attenuation, attenuation_globale);

        ajourne_contraintes_internes(
            tmp_X, d_constraints, b_constraints, W, iterations, attenuation_globale);

        /* À FAIRE : collision (position tmp_X sur la surface, réinit V). */

        integre(dt, V, X, tmp_X);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

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
    float width = 1.0f;
    float height = 1.0f;
    float min_distance = 0.05f;
    int max_attempts = 30;
    dls::math::point2f start{constantes<float>::INFINITE};
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
    auto cell_size = conf.min_distance / std::sqrt(2.0f);
    auto grid_width = static_cast<int>(std::ceil(conf.width / cell_size));
    auto grid_height = static_cast<int>(std::ceil(conf.height / cell_size));

    dls::tableau<dls::math::point2f> grid(grid_width * grid_height,
                                          dls::math::point2f(constantes<float>::INFINITE));
    dls::pile<dls::math::point2f> process;

    auto squared_distance = [](const dls::math::point2f &a, const dls::math::point2f &b) {
        auto delta_x = a.x - b.x;
        auto delta_y = a.y - b.y;

        return delta_x * delta_x + delta_y * delta_y;
    };

    auto point_around = [&conf, &random](dls::math::point2f p) {
        auto radius = random(conf.min_distance) + conf.min_distance;
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

        auto min_dist_squared = conf.min_distance * conf.min_distance;
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

    if (dls::math::sont_environ_egaux(conf.start.x, constantes<float>::INFINITE)) {
        do {
            conf.start.x = random(conf.width);
            conf.start.y = random(conf.height);
        } while (!in_area(conf.start));
    }

    add(conf.start);

    while (!process.est_vide()) {
        auto point = process.depile();

        for (int i = 0; i != conf.max_attempts; ++i) {
            auto p = point_around(point);

            if (in_area(p) && !point_too_close(p)) {
                add(p);
            }
        }
    }
}

}  // namespace bridson

template <class T>
T half(T x);

template <>
float half(float x)
{
    return 0.5f * x;
}

template <>
double half(double x)
{
    return 0.5 * x;
}

class Edge {
  public:
    using VertexType = long;

    Edge(const VertexType &p1_, const VertexType &p2_) : p1(p1_), p2(p2_), isBad(false)
    {
    }

    Edge(const Edge &e) : p1(e.p1), p2(e.p2), isBad(false)
    {
    }

    VertexType p1;
    VertexType p2;

    bool isBad;
};

inline bool operator==(const Edge &e1, const Edge &e2)
{
    return (e1.p1 == e2.p1 && e1.p2 == e2.p2) || (e1.p1 == e2.p2 && e1.p2 == e2.p1);
}

class Triangle {
  public:
    using EdgeType = Edge;
    using VertexType = long;

    Triangle(const VertexType &_p1, const VertexType &_p2, const VertexType &_p3)
        : p1(_p1), p2(_p2), p3(_p3), e1(_p1, _p2), e2(_p2, _p3), e3(_p3, _p1), isBad(false)
    {
    }

    bool containsVertex(const VertexType &v) const
    {
        return p1 == v || p2 == v || p3 == v;
    }

    VertexType p1;
    VertexType p2;
    VertexType p3;
    EdgeType e1;
    EdgeType e2;
    EdgeType e3;
    bool isBad;
};

template <class T>
inline bool operator==(Triangle const &t1, Triangle const &t2)
{
    return (t1.p1 == t2.p1 || t1.p1 == t2.p2 || t1.p1 == t2.p3) &&
           (t1.p2 == t2.p1 || t1.p2 == t2.p2 || t1.p2 == t2.p3) &&
           (t1.p3 == t2.p1 || t1.p3 == t2.p2 || t1.p3 == t2.p3);
}

class Delaunay {
  public:
    using TriangleType = Triangle;
    using EdgeType = Edge;
    using VertexType = dls::math::vec2f;

  private:
    bool cercle_circontient(TriangleType const &triangle, VertexType const &v)
    {
        auto const &p1 = _vertices[triangle.p1];
        auto const &p2 = _vertices[triangle.p2];
        auto const &p3 = _vertices[triangle.p3];

        auto const ab = longueur_carree(p1);
        auto const cd = longueur_carree(p2);
        auto const ef = longueur_carree(p3);

        auto const circum_x = (ab * (p3.y - p2.y) + cd * (p1.y - p3.y) + ef * (p2.y - p1.y)) /
                              (p1.x * (p3.y - p2.y) + p2.x * (p1.y - p3.y) + p3.x * (p2.y - p1.y));
        auto const circum_y = (ab * (p3.x - p2.x) + cd * (p1.x - p3.x) + ef * (p2.x - p1.x)) /
                              (p1.y * (p3.x - p2.x) + p2.y * (p1.x - p3.x) + p3.y * (p2.x - p1.x));

        const VertexType circum(half(circum_x), half(circum_y));
        auto const circum_radius = longueur_carree(p1 - circum);
        auto const dist = longueur_carree(v - circum);
        return dist <= circum_radius;
    }

  public:
    const dls::tableau<TriangleType> &triangulate(dls::tableau<VertexType> &vertices)
    {
        // Store the vertices locally
        _vertices = vertices;

        // Determinate the super triangle
        auto minX = vertices[0].x;
        auto minY = vertices[0].y;
        auto maxX = minX;
        auto maxY = minY;

        for (auto i = 0; i < vertices.taille(); ++i) {
            if (vertices[i].x < minX)
                minX = vertices[i].x;
            if (vertices[i].y < minY)
                minY = vertices[i].y;
            if (vertices[i].x > maxX)
                maxX = vertices[i].x;
            if (vertices[i].y > maxY)
                maxY = vertices[i].y;
        }

        auto const dx = maxX - minX;
        auto const dy = maxY - minY;
        auto const deltaMax = std::max(dx, dy);
        auto const midx = half(minX + maxX);
        auto const midy = half(minY + maxY);

        const VertexType p1(midx - 20 * deltaMax, midy - deltaMax);
        const VertexType p2(midx, midy + 20 * deltaMax);
        const VertexType p3(midx + 20 * deltaMax, midy - deltaMax);

        // std::cout << "Super triangle " << std::endl << Triangle(p1, p2, p3) << std::endl;

        // Create a list of triangles, and add the supertriangle in it
        auto offset = _vertices.taille();
        _vertices.ajoute(p1);
        _vertices.ajoute(p2);
        _vertices.ajoute(p3);

        _triangles.ajoute(TriangleType(offset + 0, offset + 1, offset + 2));

        for (auto i = 0l; i < vertices.taille(); ++i) {
            dls::tableau<EdgeType> polygone;

            for (auto &t : _triangles) {
                if (cercle_circontient(t, vertices[i])) {
                    t.isBad = true;
                    polygone.ajoute(t.e1);
                    polygone.ajoute(t.e2);
                    polygone.ajoute(t.e3);
                }
                else {
                    // message erreur?
                }
            }

            _triangles.erase(std::remove_if(_triangles.debut(),
                                            _triangles.fin(),
                                            [](TriangleType &t) { return t.isBad; }),
                             _triangles.fin());

            for (auto e1 = begin(polygone); e1 != end(polygone); ++e1) {
                for (auto e2 = e1 + 1; e2 != end(polygone); ++e2) {
                    if (*e1 == *e2) {
                        e1->isBad = true;
                        e2->isBad = true;
                    }
                }
            }

            polygone.erase(std::remove_if(begin(polygone),
                                          end(polygone),
                                          [](EdgeType &e) { return e.isBad; }),
                           end(polygone));

            for (const auto &e : polygone) {
                _triangles.ajoute(TriangleType(e.p1, e.p2, i));
            }
        }

        _triangles.erase(std::remove_if(begin(_triangles),
                                        end(_triangles),
                                        [offset](TriangleType &t) {
                                            return t.containsVertex(offset + 0) ||
                                                   t.containsVertex(offset + 1) ||
                                                   t.containsVertex(offset + 2);
                                        }),
                         end(_triangles));

        for (const auto &t : _triangles) {
            _edges.ajoute(t.e1);
            _edges.ajoute(t.e2);
            _edges.ajoute(t.e3);
        }

        // retire les trois vertices du super-triangle
        _vertices.pop_back();
        _vertices.pop_back();
        _vertices.pop_back();

        return _triangles;
    }

    const dls::tableau<TriangleType> &getTriangles() const
    {
        return _triangles;
    }
    const dls::tableau<EdgeType> &getEdges() const
    {
        return _edges;
    }
    const dls::tableau<VertexType> &getVertices() const
    {
        return _vertices;
    }

  private:
    dls::tableau<TriangleType> _triangles{};
    dls::tableau<EdgeType> _edges{};
    dls::tableau<VertexType> _vertices{};
};

class OpPatchTriangle final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Patch Triangle";
    static constexpr auto AIDE = "";

    OpPatchTriangle(Graphe &graphe_parent, Noeud &noeud_) : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(0);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_patch_triangle.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        INUTILISE(donnees_aval);

        m_corps.reinitialise();

        auto taille_x = evalue_decimal("taille_x");
        auto taille_y = evalue_decimal("taille_y");
        auto distance = evalue_decimal("distance");
        auto graine = evalue_entier("graine");
        auto centre = evalue_vecteur("centre");

        auto chef = contexte.chef;
        chef->demarre_evaluation("patch triangle");

        bridson::config conf;
        conf.min_distance = distance;
        conf.width = taille_x;
        conf.height = taille_y;
        conf.start.x = 0.0f;
        conf.start.y = 0.0f;

        auto gna = GNA(static_cast<unsigned long>(graine));

        auto vertices = dls::tableau<dls::math::vec2f>();

        bridson::poisson_disc_distribution(
            conf,
            // random
            [&gna](float range) { return gna.uniforme(0.0f, range); },
            // in_area
            [&](dls::math::point2f const &p) {
                return p.x > 0 && p.x < taille_x && p.y > 0 && p.y < taille_y;
            },
            // output
            [&vertices](dls::math::point2f const &p) {
                // m_corps.ajoute_point(p.x, 0.0f, p.y);
                vertices.ajoute(dls::math::vec2f(p.x, p.y));
            });

        chef->indique_progression(45.0f);

        auto delaunay = Delaunay();
        delaunay.triangulate(vertices);

        chef->indique_progression(90.0f);

        auto const &vertex = delaunay.getVertices();
        auto points = m_corps.points_pour_ecriture();

        for (auto const &v : vertex) {
            auto p = centre;
            p.x += v.x - (taille_x / 2.0f);
            p.z += v.y - (taille_y / 2.0f);

            points.ajoute_point(p.x, p.y, p.z);
        }

        auto const &cotes = delaunay.getTriangles();

        for (auto const &cote : cotes) {
            auto prim = m_corps.ajoute_polygone(type_polygone::FERME, 3);
            m_corps.ajoute_sommet(prim, static_cast<long>(cote.p1));
            m_corps.ajoute_sommet(prim, static_cast<long>(cote.p2));
            m_corps.ajoute_sommet(prim, static_cast<long>(cote.p3));
        }

        chef->indique_progression(100.0f);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_operatrices_vetement(UsineOperatrice &usine)
{
    usine.enregistre_type(cree_desc<OperatriceSimVetement>());
    usine.enregistre_type(cree_desc<OpPatchTriangle>());
}

#pragma clang diagnostic pop
