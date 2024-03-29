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

#include "operatrices_visualisation.hh"

#include "biblinternes/outils/gna.hh"

#include "corps/iteration_corps.hh"
#include "corps/limites_corps.hh"
#include "corps/volume.hh"

#include "coeur/delegue_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "wolika/grille_eparse.hh"

#include "arbre_octernaire.hh"
#include "outils_visualisation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static void rassemble_topologie(arbre_octernaire::noeud const *noeud,
                                Corps &corps,
                                bool dessine_branches,
                                bool dessine_feuilles)
{
    dls::math::vec3f couleurs[2] = {
        dls::math::vec3f(0.0f, 1.0f, 0.0f),
        dls::math::vec3f(0.0f, 0.0f, 1.0f),
    };

    auto const &min = noeud->limites.min;
    auto const &max = noeud->limites.max;

    if (noeud->est_feuille) {
        if (dessine_feuilles) {
            dessine_boite(corps, corps.attribut("C"), min, max, couleurs[0]);
        }

        return;
    }
    else if (dessine_branches) {
        dessine_boite(corps, corps.attribut("C"), min, max, couleurs[1]);
    }

    for (int i = 0; i < arbre_octernaire::NOMBRE_ENFANTS; ++i) {
        if (noeud->enfants[i] == nullptr) {
            continue;
        }

        rassemble_topologie(noeud->enfants[i], corps, dessine_branches, dessine_feuilles);
    }
}

static void rassemble_topologie(arbre_octernaire const &arbre,
                                Corps &corps,
                                bool dessine_branches,
                                bool dessine_feuilles)
{
    corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);
    rassemble_topologie(arbre.racine(), corps, dessine_branches, dessine_feuilles);
}

static void colore_prims(arbre_octernaire::noeud const *noeud,
                         Corps &corps,
                         Attribut *attr,
                         GNA &gna)
{
    if (noeud->est_feuille) {
        auto couleur = dls::math::vec3f();
        couleur.x = gna.uniforme(0.0f, 1.0f);
        couleur.y = gna.uniforme(0.0f, 1.0f);
        couleur.z = gna.uniforme(0.0f, 1.0f);

        for (auto ref : noeud->refs) {
            /* Une même primitive peut être dans plusieurs noeuds... */
            assigne(attr->r32(ref), couleur);
        }

        return;
    }

    for (int i = 0; i < arbre_octernaire::NOMBRE_ENFANTS; ++i) {
        if (noeud->enfants[i] == nullptr) {
            continue;
        }

        colore_prims(noeud->enfants[i], corps, attr, gna);
    }
}

class OperatriceVisualisationArbreOcternaire final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Visualisation Arbre Octernaire";
    static constexpr auto AIDE = "";

    OperatriceVisualisationArbreOcternaire(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_vis_arbre_hbe.jo"};
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

        if (!valide_corps_entree(*this, corps_entree, true, true)) {
            return res_exec::ECHOUEE;
        }

        auto dessine_branches = evalue_bool("dessine_branches");
        auto dessine_feuilles = evalue_bool("dessine_feuilles");
        auto type_visualisation = evalue_enum("type_visualisation");

        auto delegue = delegue_arbre_octernaire(*corps_entree);
        auto arbre = arbre_octernaire::construit(delegue);

        if (type_visualisation == "topologie") {
            if (dessine_branches || dessine_feuilles) {
                rassemble_topologie(arbre, m_corps, dessine_branches, dessine_feuilles);
            }
        }
        else if (type_visualisation == "noeuds_colorés") {
            corps_entree->copie_vers(&m_corps);
            auto attr = m_corps.attribut("C");

            if (attr == nullptr) {
                attr = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::PRIMITIVE);
            }
            else if (attr->portee != portee_attr::PRIMITIVE) {
                m_corps.supprime_attribut("C");
                attr = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::PRIMITIVE);
            }

            auto gna = GNA{};

            colore_prims(arbre.racine(), m_corps, attr, gna);
        }

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

struct delegue_arbre_bvh {
    Corps const &corps;

    delegue_arbre_bvh(Corps const &c) : corps(c)
    {
    }

    int nombre_elements() const
    {
        return static_cast<int>(corps.prims()->taille());
    }

    void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
    {
        auto poly = dynamic_cast<Polygone *>(corps.prims()->prim(idx));

        cos.efface();
        cos.reserve(poly->nombre_sommets());

        auto points = corps.points_pour_lecture();

        for (auto i = 0; i < poly->nombre_sommets(); ++i) {
            cos.ajoute(points.point_monde(poly->index_point(i)));
        }
    }
};

static auto rassemble_topologie(bli::BVHTree &arbre,
                                Corps &corps,
                                bool dessine_branches,
                                bool dessine_feuilles)
{
    dls::math::vec3f couleurs[2] = {
        dls::math::vec3f(0.0f, 1.0f, 0.0f),
        dls::math::vec3f(0.0f, 0.0f, 1.0f),
    };

    auto attr_C = corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);

    for (auto const &noeud : arbre.nodearray) {
        auto bv = noeud.bv;

        auto min = dls::math::vec3f(bv[0], bv[2], bv[4]);
        auto max = dls::math::vec3f(bv[1], bv[3], bv[5]);

        if (noeud.totnode == 0 && dessine_feuilles) {
            dessine_boite(corps, attr_C, min, max, couleurs[0]);
        }
        else if (noeud.totnode != 0 && dessine_branches) {
            dessine_boite(corps, attr_C, min, max, couleurs[1]);
        }
    }
}

/* ************************************************************************** */

static auto rassemble_topologie(ArbreHBE &arbre,
                                Corps &corps,
                                bool dessine_branches,
                                bool dessine_feuilles)
{
    dls::math::vec3f couleurs[2] = {
        dls::math::vec3f(0.0f, 1.0f, 0.0f),
        dls::math::vec3f(0.0f, 0.0f, 1.0f),
    };

    auto attr_C = corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);

    for (auto const &noeud : arbre.noeuds) {
        auto const &min = dls::math::converti_type_vecteur<float>(noeud.limites.min);
        auto const &max = dls::math::converti_type_vecteur<float>(noeud.limites.max);

        if (noeud.est_feuille() && dessine_feuilles) {
            dessine_boite(corps, attr_C, min, max, couleurs[0]);
        }
        else if (!noeud.est_feuille() && dessine_branches) {
            dessine_boite(corps, attr_C, min, max, couleurs[1]);
        }
    }
}

class OperatriceVisualisationArbreBVH final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Visualisation Arbre BVH";
    static constexpr auto AIDE = "";

    OperatriceVisualisationArbreBVH(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_vis_arbre_hbe.jo"};
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

        auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

        if (!valide_corps_entree(*this, corps_entree, true, true)) {
            return res_exec::ECHOUEE;
        }

        auto dessine_branches = evalue_bool("dessine_branches");
        auto dessine_feuilles = evalue_bool("dessine_feuilles");
        auto type_visualisation = evalue_enum("type_visualisation");

        auto delegue_prims = delegue_arbre_bvh(*corps_entree);
        auto arbre_hbe = bli::cree_arbre_bvh(delegue_prims);

        if (type_visualisation == "topologie") {
            if (dessine_branches || dessine_feuilles) {
                rassemble_topologie(*arbre_hbe, m_corps, dessine_branches, dessine_feuilles);
            }
        }
        else if (type_visualisation == "noeuds_colorés") {
            //			corps_entree->copie_vers(&m_corps);

            //			auto attr = m_corps.attribut("C");

            //			if (attr == nullptr) {
            //				attr = m_corps.ajoute_attribut("C", type_attribut::R32, 3,
            //portee_attr::PRIMITIVE);
            //			}
            //			else if (attr->portee != portee_attr::PRIMITIVE) {
            //				m_corps.supprime_attribut("C");
            //				attr = m_corps.ajoute_attribut("C", type_attribut::R32, 3,
            //portee_attr::PRIMITIVE);
            //			}

            //			auto gna = GNA{};

            //			for (auto noeud : arbre_hbe.noeuds) {
            //				if (!noeud.est_feuille()) {
            //					continue;
            //				}

            //				auto couleur = dls::math::vec3f();
            //				couleur.x = gna.uniforme(0.0f, 1.0f);
            //				couleur.y = gna.uniforme(0.0f, 1.0f);
            //				couleur.z = gna.uniforme(0.0f, 1.0f);

            //				for (auto i = 0; i < noeud.nombre_references; ++i) {
            //					auto id_prim = arbre_hbe.index_refs[noeud.decalage_reference + i];
            //					attr->r32(id_prim) = couleur;
            //				}
            //			}
        }

        memoire::deloge("BVHTree", arbre_hbe);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

template <typename T>
static auto visualise_topologie(Corps &corps, wlk::grille_eparse<T> const &grille)
{
    auto limites = grille.desc().etendue;
    auto attr_C = corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);
    auto plg = grille.plage();

    while (!plg.est_finie()) {
        auto tuile = plg.front();
        plg.effronte();

        auto min_tuile = grille.index_vers_monde(tuile->min);
        auto max_tuile = grille.index_vers_monde(tuile->max);

        dessine_boite(corps, attr_C, min_tuile, max_tuile, dls::math::vec3f(0.1f, 0.1f, 0.8f));
    }

    dessine_boite(corps, attr_C, limites.min, limites.max, dls::math::vec3f(0.1f, 0.8f, 0.1f));
}

class OpVisualiseGrilleEparse final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Visualisation Grille Éparse";
    static constexpr auto AIDE = "";

    OpVisualiseGrilleEparse(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
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
        auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

        if (!valide_corps_entree(*this, corps_entree, false, false)) {
            return res_exec::ECHOUEE;
        }

        if (!possede_volume(*corps_entree)) {
            this->ajoute_avertissement("Le Corps ne possède pas de volume");
            return res_exec::ECHOUEE;
        }

        auto prims = corps_entree->prims();
        auto volume = static_cast<Volume *>(nullptr);

        for (auto i = 0; i < prims->taille(); ++i) {
            auto prim = prims->prim(i);

            if (prim->type_prim() == type_primitive::VOLUME) {
                volume = dynamic_cast<Volume *>(prim);
            }
        }

        auto grille = volume->grille;

        if (!grille->est_eparse()) {
            this->ajoute_avertissement("Le volume n'est pas épars");
            return res_exec::ECHOUEE;
        }

        auto grille_eprs = dynamic_cast<wlk::grille_eparse<float> *>(grille);

        visualise_topologie(m_corps, *grille_eprs);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_operatrices_visualisation(UsineOperatrice &usine)
{
    usine.enregistre_type(cree_desc<OperatriceVisualisationArbreOcternaire>());
    usine.enregistre_type(cree_desc<OperatriceVisualisationArbreBVH>());
    usine.enregistre_type(cree_desc<OpVisualiseGrilleEparse>());
}

#pragma clang diagnostic pop
