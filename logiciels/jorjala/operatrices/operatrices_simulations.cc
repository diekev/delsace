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

#include "operatrices_simulations.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/moultfilage/synchronise.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/delegue_hbe.hh"
#include "coeur/graphe.hh"
#include "coeur/operatrice_simulation.hh"
#include "coeur/usine_operatrice.h"

#include "corps/groupes.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceEntreeSimulation final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Entrée Simulation";
    static constexpr auto AIDE = "";

    OperatriceEntreeSimulation(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(0);
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
        INUTILISE(contexte);
        INUTILISE(donnees_aval);

        m_corps.reinitialise();

        if (m_graphe_parent.donnees.est_vide()) {
            ajoute_avertissement("Les données du graphe sont vides !");
            return res_exec::ECHOUEE;
        }

        auto corps = std::any_cast<Corps *>(m_graphe_parent.donnees[0]);
        corps->copie_vers(&m_corps);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

class OperatriceGravite final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Gravité";
    static constexpr auto AIDE = "";

    OperatriceGravite(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_gravite.jo"};
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

        auto liste_points = m_corps.points_pour_lecture();
        auto const nombre_points = liste_points.taille();
        auto attrf = m_corps.ajoute_attribut("F", type_attribut::R32, 3, portee_attr::POINT);

        auto gravite = evalue_vecteur("gravité", contexte.temps_courant);

        /* À FAIRE : f = m * a => multiplier par la masse? */
        boucle_parallele(tbb::blocked_range<long>(0, nombre_points),
                         [&](tbb::blocked_range<long> const &plage) {
                             for (auto i = plage.begin(); i < plage.end(); ++i) {
                                 assigne(attrf->r32(i), gravite);
                             }
                         });

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

/* À FAIRE : manipulatrice dédiée pour la position/orientation. */
class OperatriceVent final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Vent";
    static constexpr auto AIDE = "";

    OperatriceVent(Graphe &graphe_parent, Noeud &noeud_) : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_vent.jo"};
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

        auto liste_points = m_corps.points_pour_lecture();
        auto const nombre_points = liste_points.taille();
        auto attrf = m_corps.ajoute_attribut("F", type_attribut::R32, 3, portee_attr::POINT);

        auto direction = evalue_vecteur("direction", contexte.temps_courant);
        auto amplitude = evalue_decimal("amplitude", contexte.temps_courant);

        auto force_max = direction * amplitude;

        /* À FAIRE : vent
         * - intégration des forces dans le désordre (gravité après vent) par
         *   exemple en utilisant un attribut extra.
         * - turbulence
         */
        boucle_parallele(tbb::blocked_range<long>(0, nombre_points),
                         [&](tbb::blocked_range<long> const &plage) {
                             for (auto i = plage.begin(); i < plage.end(); ++i) {
                                 auto force = attrf->r32(i);

                                 for (size_t j = 0; j < 3; ++j) {
                                     force[j] = std::min(force_max[j], force[j] + force_max[j]);
                                 }
                             }
                         });

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

class OperatriceSolveurParticules final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Solveur Particules";
    static constexpr auto AIDE = "";

    OperatriceSolveurParticules(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{""};
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

        auto liste_points = m_corps.points_pour_ecriture();
        auto const nombre_points = liste_points.taille();
        auto attr_P = m_corps.ajoute_attribut(
            "pos_pre", type_attribut::R32, 3, portee_attr::POINT);
        auto attrf = m_corps.ajoute_attribut("F", type_attribut::R32, 3, portee_attr::POINT);

        /* À FAIRE : passe le temps par image en paramètre. */
        auto const temps_par_image = 1.0f / 24.0f;
        /* À FAIRE : masse comme propriété des particules */
        auto const masse = 1.0f;  // evalfloat("masse");
        auto const masse_inverse = 1.0f / masse;

        /* ajoute attribut vélocité */
        auto attr_V = m_corps.ajoute_attribut("V", type_attribut::R32, 3, portee_attr::POINT);

        /* Ajourne la position des particules selon les équations :
         * v(t) = F(t)dt
         * x(t) = v(t)dt
         *
         * Voir :
         * Physically Based Modeling: Principles and Practice
         * https://www.cs.cmu.edu/~baraff/sigcourse/
         */

        auto attr_desactiv = m_corps.ajoute_attribut(
            "part_desactiv", type_attribut::Z8, 1, portee_attr::POINT);

        boucle_parallele(tbb::blocked_range<long>(0, nombre_points),
                         [&](tbb::blocked_range<long> const &plage) {
                             for (long i = plage.begin(); i < plage.end(); ++i) {
                                 auto desactivee = attr_desactiv->z8(i)[0];

                                 if (desactivee == 1) {
                                     continue;
                                 }

                                 auto pos = liste_points.point_local(i);

                                 /* a = f / m */
                                 auto f = dls::math::vec3f();
                                 extrait(attrf->r32(i), f);
                                 auto const acceleration = f * masse_inverse;

                                 /* velocite = acceleration * temp_par_image + velocite */
                                 extrait(attr_V->r32(i), f);
                                 auto velocite = f + acceleration * temps_par_image;

                                 /* position = velocite * temps_par_image + position */
                                 auto npos = pos + velocite * temps_par_image;

                                 liste_points.point(i, npos);
                                 assigne(attr_V->r32(i), velocite);
                                 assigne(attr_P->r32(i), pos);
                                 assigne(attrf->r32(i), dls::math::vec3f(0.0f));
                             }
                         });

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

enum rep_collision {
    RIEN,
    REBONDIS,
    COLLE,
};

class OperatriceCollision final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Collision";
    static constexpr auto AIDE = "";

    OperatriceCollision(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_collision.jo"};
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
        auto corps_collision = entree(1)->requiers_corps(contexte, donnees_aval);

        if (!valide_corps_entree(*this, corps_collision, true, true, 1)) {
            return res_exec::ECHOUEE;
        }

        auto const prims_collision = corps_collision->prims();
        auto const points_collision = corps_collision->points_pour_lecture();

        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto liste_points = m_corps.points_pour_ecriture();
        auto const nombre_points = liste_points.taille();

        auto const elasticite = evalue_decimal("élasticité", contexte.temps_courant);
        /* À FAIRE : rayon comme propriété des particules */
        auto const rayon = evalue_decimal("rayon", contexte.temps_courant);

        /* ajoute attribut vélocité */
        auto attr_V = m_corps.attribut("V");

        if (attr_V == nullptr) {
            ajoute_avertissement("Aucune attribut de vélocité trouvé !");
            return res_exec::ECHOUEE;
        }

        auto attr_P = m_corps.attribut("pos_pre");

        if (attr_P == nullptr) {
            ajoute_avertissement("Aucune attribut de position trouvé !");
            return res_exec::ECHOUEE;
        }

        auto const chaine_reponse = evalue_enum("réponse_collision");
        rep_collision reponse;

        if (chaine_reponse == "rien") {
            reponse = rep_collision::RIEN;
        }
        else if (chaine_reponse == "rebondis") {
            reponse = rep_collision::REBONDIS;
        }
        else if (chaine_reponse == "colle") {
            reponse = rep_collision::COLLE;
        }
        else {
            this->ajoute_avertissement("Opération '", chaine_reponse, "' inconnue");
            return res_exec::ECHOUEE;
        }

        auto chef = contexte.chef;
        chef->demarre_evaluation("maillage vers volume");

        auto groupe = m_corps.ajoute_groupe_point("collision");
        groupe->reinitialise();

        auto groupe_sync = dls::synchronise<GroupePoint *>();
        groupe_sync = groupe;

        auto attr_desactiv = m_corps.ajoute_attribut(
            "part_desactiv", type_attribut::Z8, 1, portee_attr::POINT);

        auto delegue_prims = DeleguePrim(*corps_collision);
        auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

        boucle_parallele(
            tbb::blocked_range<long>(0, nombre_points),
            [&](tbb::blocked_range<long> const &plage) {
                for (long i = plage.begin(); i < plage.end(); ++i) {
                    auto pos_cou = liste_points.point_local(i);
                    auto vel = dls::math::vec3f();
                    extrait(attr_V->r32(i), vel);
                    auto pos_pre = dls::math::vec3f();
                    extrait(attr_P->r32(i), pos_pre);
                    auto desactivee = attr_desactiv->z8(i)[0];

                    if (desactivee == 1) {
                        continue;
                    }

                    /* Calcul la position en espace objet. */
                    auto pos_monde = m_corps.transformation(dls::math::point3d(pos_pre));

                    auto rayon_part = dls::phys::rayond{};
                    rayon_part.origine = pos_monde;
                    rayon_part.distance_max = 1000.0;

                    auto dir = normalise(pos_cou - pos_pre);
                    rayon_part.direction = dls::math::converti_type<double>(dir);
                    calcul_direction_inverse(rayon_part);

                    for (size_t j = 0; j < 3; ++j) {
                        rayon_part.direction_inverse[j] = 1.0 / rayon_part.direction[j];
                    }

                    auto const &esect = traverse(arbre_hbe, delegue_prims, rayon_part);

                    if (!esect.touche) {
                        continue;
                    }

                    if (esect.distance > static_cast<double>(rayon)) {
                        continue;
                    }

                    auto index_prim = esect.idx;

                    groupe_sync.accede_ecriture(
                        [&](GroupePoint *groupe_point) { groupe_point->ajoute_index(i); });

                    switch (reponse) {
                        case rep_collision::RIEN:
                        {
                            break;
                        }
                        case rep_collision::REBONDIS:
                        {
                            auto prim = prims_collision->prim(index_prim);
                            auto poly = dynamic_cast<Polygone *>(prim);
                            auto const &v0 = points_collision.point_local(poly->index_point(0));
                            auto const &v1 = points_collision.point_local(poly->index_point(1));
                            auto const &v2 = points_collision.point_local(poly->index_point(2));

                            auto const e1 = v1 - v0;
                            auto const e2 = v2 - v0;
                            auto nor_poly = normalise(produit_croix(e1, e2));

                            /* Trouve le normal de la vélocité au point de collision. */
                            auto nv = dls::math::produit_scalaire(nor_poly, vel) * nor_poly;

                            /* Trouve la tangente de la vélocité. */
                            auto tv = vel - nv;

                            /* Le normal de la vélocité est multiplité par le coefficient
                             * d'élasticité. */
                            vel = -elasticite * nv + tv;
                            assigne(attr_V->r32(i), vel);
                            break;
                        }
                        case rep_collision::COLLE:
                        {
                            pos_cou = dls::math::converti_type_vecteur<float>(esect.point);

                            liste_points.point(i, pos_cou);
                            assigne(attr_V->r32(i), dls::math::vec3f(0.0f));
                            assigne(attr_desactiv->z8(i), char(1));
                            break;
                        }
                    }
                }

                auto delta = static_cast<float>(plage.end() - plage.begin());
                auto total = static_cast<float>(nombre_points);

                chef->indique_progression_parallele(delta / total * 100.0f);
            });

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

class BarnesHutSummation {
  public:
    struct Particule {
        dls::math::vec3f position = dls::math::vec3f(0.0f);
        float mass = 0.0f;

        Particule() = default;

        Particule(dls::math::vec3f const &pos, float m) : position(pos), mass(m)
        {
        }

        Particule operator+(const Particule &o) const
        {
            Particule ret;
            ret.mass = mass + o.mass;
            ret.position = (position * mass + o.position * o.mass) * (1.0f / ret.mass);
            return ret;
        }
    };

  protected:
    struct Node {
        Particule p{};
        int children[8] = {
            0, 0, 0, 0, 0, 0, 0, 0};  // We do not use pointer here to save memory bandwidth(64
        // bit v.s. 32 bit)
        // There are so many ways to optimize and I'll consider them later...
        dls::math::vec3i bounds[2];  // À FAIRE: this is quite brute-force...

        Node() = default;

        bool is_leaf()
        {
            for (int i = 0; i < 8; i++) {
                if (children[i] != 0) {
                    return false;
                }
            }

            return p.mass > 0;
        }
    };

    float resolution = 0.0f;
    float inv_resolution = 0.0f;
    int total_levels = 0;
    int margin = 0;

    dls::tableau<Node> nodes{};
    int node_end = 0;
    dls::math::vec3f lower_corner{};

    dls::math::vec3i coordonnees_grille(const dls::math::vec3f &position)
    {
        dls::math::vec3i u;
        dls::math::vec3f t = (position - lower_corner) * inv_resolution;
        for (size_t i = 0; i < 3; i++) {
            u[i] = int(t[i]);
        }
        return u;
    }

    int calcul_index_enfant(const dls::math::vec3i &u, int level)
    {
        int ret = 0;
        for (size_t i = 0; i < 3; i++) {
            ret += ((u[i] >> (total_levels - level - 1)) & 1) << i;
        }
        return ret;
    }

    void sommarise(int t)
    {
        auto &node = nodes[t];

        if (node.is_leaf()) {
            auto u = coordonnees_grille(node.p.position);
            node.bounds[0] = u - dls::math::vec3i(margin);
            node.bounds[1] = u + dls::math::vec3i(margin);
            return;
        }

        float mass = 0.0f;
        dls::math::vec3f total_position(0.0f);
        node.bounds[0] = dls::math::vec3i(std::numeric_limits<int>::max());
        node.bounds[1] = dls::math::vec3i(std::numeric_limits<int>::min());

        for (int c = 0; c < 8; c++) {
            if (node.children[c]) {
                sommarise(nodes[t].children[c]);
                auto const &ch = nodes[node.children[c]];
                mass += ch.p.mass;
                total_position += ch.p.mass * ch.p.position;

                for (size_t i = 0; i < 3; i++) {
                    node.bounds[0][i] = std::min(node.bounds[0][i], ch.bounds[0][i]);
                    node.bounds[1][i] = std::max(node.bounds[1][i], ch.bounds[1][i]);
                }
            }
        }

        total_position *= dls::math::vec3f(1.0f / mass);

        if (!dls::math::est_fini(total_position)) {
            /* sérialise les données */
        }

        node.p = Particule(total_position, mass);
    }

    int cree_enfant(int t, int child_index)
    {
        return cree_enfant(t, child_index, Particule(dls::math::vec3f(0.0f), 0.0f));
    }

    int cree_enfant(int t, int child_index, const Particule &p)
    {
        int nt = cree_nouveau_noeud();
        nodes[t].children[child_index] = nt;
        nodes[nt].p = p;
        return nt;
    }

    int cree_nouveau_noeud()
    {
        nodes[node_end] = Node();
        return node_end++;
    }

  public:
    // We do not evaluate the weighted average of position and mass on the fly
    // for efficiency and accuracy
    void initialise(float res, float marginfloat, const dls::tableau<Particule> &particles)
    {
        this->resolution = res;
        this->inv_resolution = 1.0f / res;
        this->margin = static_cast<int>(std::ceil(marginfloat * inv_resolution));
        assert(particles.taille() != 0);
        dls::math::vec3f lower(1e30f);
        dls::math::vec3f upper(-1e30f);

        for (auto &p : particles) {
            for (size_t k = 0; k < 3; k++) {
                lower[k] = std::min(lower[k], p.position[k]);
                upper[k] = std::max(upper[k], p.position[k]);
            }
            // TC_P(p.position);
        }

        lower_corner = lower;
        int intervals = static_cast<int>(std::ceil(max(upper - lower) / res));
        total_levels = 0;
        for (int i = 1; i < intervals; i *= 2, total_levels++)
            ;
        // We do not use the 0th node...
        node_end = 1;
        nodes.efface();
        nodes.redimensionne(particles.taille() * 2);
        int root = cree_nouveau_noeud();
        // Make sure that one leaf node contains only one particle.
        // Unless particles are too close and thereby merged.
        for (auto &p : particles) {
            if (p.mass == 0.0f) {
                continue;
            }

            dls::math::vec3i u = coordonnees_grille(p.position);
            auto t = root;

            if (nodes[t].is_leaf()) {
                // First node
                nodes[t].p = p;
                continue;
            }

            // Traverse down until there's no way...
            int k = 0;
            // TC_ERROR("cp maybe originally used without initialization");
            int cp = -1;
            for (; k < total_levels; k++) {
                cp = calcul_index_enfant(u, k);
                if (nodes[t].children[cp] != 0) {
                    t = nodes[t].children[cp];
                }
                else {
                    break;
                }
            }
            if (nodes[t].is_leaf()) {
                // Leaf node, containing one particle q
                // Split the node until p and q belong to different children.
                Particule q = nodes[t].p;
                nodes[t].p = Particule();
                dls::math::vec3i v = coordonnees_grille(q.position);
                int cq = calcul_index_enfant(v, k);
                while (cp == cq && k < total_levels) {
                    t = cree_enfant(t, cp);
                    k++;
                    cp = calcul_index_enfant(u, k);
                    cq = calcul_index_enfant(v, k);
                }
                if (k == total_levels) {
                    // We have to merge two particles since they are too close...
                    q = p + q;
                    cree_enfant(t, cp, q);
                }
                else {
                    nodes[t].p = Particule();
                    cree_enfant(t, cp, p);
                    cree_enfant(t, cq, q);
                }
            }
            else {
                // Non-leaf node, simply create a child.
                cree_enfant(t, cp, p);
            }
        }
        // TC_P(node_end);
        sommarise(root);
    }

    /*
  template<typename T>
  dls::math::vec3f summation(const Particle &p, const T &func) {
      // À FAIRE: fine level
      // À FAIRE: only one particle?
      int t = 1;
      dls::math::vec3f ret(0.0f);
      dls::math::vec3f u = get_coord(p.position);
      for (int k = 0; k < total_levels; k++) {
          int cp = get_child_index(u, k);
          for (int c = 0; c < 8; c++) {
              if (c != cp && nodes[t].children[c]) {
                  const Node &n = nodes[nodes[t].children[c]];
                  auto tmp = func(p, n.p);
                  ret += tmp;
              }
          }
          t = nodes[t].children[cp];
          if (t == 0) {
              break;
          }
      }
      return ret;
  }
  */

    template <typename T>
    dls::math::vec3f summation(int t, const Particule &p, const T &func)
    {
        const Node &node = nodes[t];
        if (nodes[t].is_leaf()) {
            return func(p, node.p);
        }

        dls::math::vec3f ret(0.0f);
        dls::math::vec3i u = coordonnees_grille(p.position);

        for (size_t c = 0; c < 8; c++) {
            if (node.children[c]) {
                const Node &ch = nodes[node.children[c]];
                if (ch.bounds[0][0] <= u[0] && u[0] <= ch.bounds[1][0] &&
                    ch.bounds[0][1] <= u[1] && u[1] <= ch.bounds[1][1] &&
                    ch.bounds[0][2] <= u[2] && u[2] <= ch.bounds[1][2]) {
                    ret += summation(node.children[c], p, func);
                }
                else {
                    // Coarse summation
                    ret += func(p, ch.p);
                }
            }
        }

        return ret;
    }
};

class OperatriceSolveurNCorps final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Solveur N-Corps";
    static constexpr auto AIDE = "";

    OperatriceSolveurNCorps(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_solveur_n_corps.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    void initialise_attributs()
    {
        auto liste_points = m_corps.points_pour_lecture();
        auto const nombre_points = liste_points.taille();

        auto attr_V = m_corps.attribut("V");
        auto mult_vel = evalue_decimal("mult_vel");

        if (attr_V == nullptr) {
            attr_V = m_corps.ajoute_attribut("V", type_attribut::R32, 3, portee_attr::POINT);

            for (auto i = 0; i < nombre_points; ++i) {
                auto pos = liste_points.point_local(i);
                assigne(attr_V->r32(i), (pos - dls::math::vec3f(0.5f)) * mult_vel);
            }
        }

        m_corps.ajoute_attribut("pos_pre", type_attribut::R32, 3, portee_attr::POINT);
    }

    void sous_etape(float gravitation, float dt)
    {
        auto liste_points = m_corps.points_pour_ecriture();
        auto const nombre_points = liste_points.taille();
        auto attr_V = m_corps.attribut("V");
        auto attr_P = m_corps.attribut("pos_pre");

        using BHP = BarnesHutSummation::Particule;

        dls::tableau<BHP> bhps;
        bhps.reserve(nombre_points);

        for (auto i = 0; i < nombre_points; ++i) {
            auto pos = liste_points.point_local(i);
            bhps.ajoute(BHP(pos, 1.0f));
        }

        BarnesHutSummation bhs;
        bhs.initialise(1e-4f, 1e-3f, bhps);

        auto f = [](const BHP &p, const BHP &q) {
            dls::math::vec3f d = p.position - q.position;
            float dist2 = produit_scalaire(d, d);
            dist2 += 1e-4f;
            d *= dls::math::vec3f(p.mass * q.mass / (dist2 * std::sqrt(dist2)));
            return d;
        };

        if (gravitation != 0.0f) {
            boucle_parallele(tbb::blocked_range<long>(0, nombre_points),
                             [&](tbb::blocked_range<long> const &plage) {
                                 for (auto i = plage.begin(); i < plage.end(); ++i) {
                                     auto pos = liste_points.point_local(i);
                                     dls::math::vec3f totalf_bhs = bhs.summation(
                                         1, BHP(pos, 1.0f), f);

                                     auto v = dls::math::vec3f();
                                     extrait(attr_V->r32(i), v);

                                     assigne(attr_V->r32(i), v + totalf_bhs * gravitation * dt);
                                 }
                             });
        }

        for (auto i = 0; i < nombre_points; ++i) {
            auto pos = liste_points.point_local(i);

            auto v = dls::math::vec3f();
            extrait(attr_V->r32(i), v);
            liste_points.point(i, pos + dt * v);
            assigne(attr_P->r32(i), pos);
        }
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto liste_points = m_corps.points_pour_lecture();
        auto const nombre_points = liste_points.taille();

        if (nombre_points == 0) {
            return res_exec::REUSSIE;
        }

        initialise_attributs();

        auto const dt_config = 0.1f;  // evalue_decimal("dt");
        auto const dt_simulation = evalue_decimal("dt_simulation");
        auto nb_etape = dt_config / dt_simulation;

        auto const gravitation = evalue_decimal("gravitation");

        for (auto i = 0; i < static_cast<int>(nb_etape); ++i) {
            sous_etape(gravitation, dt_simulation / nb_etape);
        }

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_operatrices_simulations(UsineOperatrice &usine)
{
    usine.enregistre_type(cree_desc<OperatriceSimulation>());

    usine.enregistre_type(cree_desc<OperatriceEntreeSimulation>());
    usine.enregistre_type(cree_desc<OperatriceGravite>());
    usine.enregistre_type(cree_desc<OperatriceSolveurParticules>());
    usine.enregistre_type(cree_desc<OperatriceCollision>());
    usine.enregistre_type(cree_desc<OperatriceVent>());
    usine.enregistre_type(cree_desc<OperatriceSolveurNCorps>());
}

#pragma clang diagnostic pop
