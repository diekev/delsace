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

#include "execution.hh"

#include <iostream>
#include <tbb/task.h>
#include <tbb/tick_count.h>

#include "coeur/composite.h"
#include "coeur/contexte_evaluation.hh"
#include "coeur/evenement.h"
#include "coeur/jorjala.hh"
#include "coeur/noeud_image.h"
#include "coeur/objet.h"
#include "coeur/operatrice_image.h"
#include "coeur/tache.h"

#include "reseau.hh"

#undef LOG_EVALUATION

#ifdef LOG_EVALUATION
static bool GV_log_evaluation = true;
#else
static bool GV_log_evaluation = false;
#endif

#define DEBUT_LOG_EVALUATION                                                                      \
    if (GV_log_evaluation) {                                                                      \
        std::cerr
#define FIN_LOG_EVALUATION                                                                        \
    '\n';                                                                                         \
    }

/* À FAIRE, évaluation asynchrone :
 * - TBB semblerait avoir du mal avec les tâches trop rapides, il arrive qu'au
 *   bout d'un moment les tâches ne sont plus exécutées, car les threads n'ont
 *   pas eu le temps de se joindre.
 * - si une tâche est en cours, ajout des nouvelles tâches dans une queue et
 *   retourne la première de celles-ci dans tbb::task::execute().
 * - une tâche est lancée au démarrage, pourquoi ?
 */

/* ************************************************************************** */

class TacheJorjala : public tbb::task {
  protected:
    TaskNotifier notifier;
    Jorjala &m_jorjala;

  public:
    explicit TacheJorjala(Jorjala &jorjala);

    virtual void evalue() = 0;

    tbb::task *execute() override;
};

TacheJorjala::TacheJorjala(Jorjala &jorjala)
    : notifier(jorjala.fenetre_principale), m_jorjala(jorjala)
{
}

tbb::task *TacheJorjala::execute()
{
    m_jorjala.interrompu = false;
    notifier.signale_debut_tache();
    evalue();
    notifier.signale_fin_tache();

    m_jorjala.tache_en_cours = false;
    return nullptr;
}

/* ************************************************************************** */

static void evalue_objet(ContexteEvaluation const &contexte, Objet *objet)
{
    if (objet == nullptr) {
        std::cerr << "ERREUR : l'objet est nul !\n";
        return;
    }

    auto &graphe = objet->noeud->graphe;
    auto noeud_sortie = graphe.dernier_noeud_sortie;

    if (noeud_sortie == nullptr) {
        return;
    }

    auto operatrice = extrait_opimage(noeud_sortie->donnees);

    auto const t0 = tbb::tick_count::now();

    operatrice->reinitialise_avertisements();
    operatrice->execute(contexte, nullptr);

    auto const t1 = tbb::tick_count::now();
    auto const delta = (t1 - t0).seconds();
    noeud_sortie->temps_execution = static_cast<float>(delta);

    /* on garde une copie pour l'évaluation dans des threads séparés, copie
     * nécessaire pour pouvoir rendre l'objet dans la vue quand le rendu prend
     * plus de temps que l'évaluation asynchrone. */
    objet->donnees.accede_ecriture([operatrice](DonneesObjet *donnees_objet) {
        auto &_corps_ = extrait_corps(donnees_objet);
        _corps_.reinitialise();
        operatrice->corps()->copie_vers(&_corps_);
    });

#if 0 /* À FAIRE : interface pour la transformation */
	auto position = dls::math::point3f(evalue_vecteur("position", contexte.temps_courant));
	auto rotation = evalue_vecteur("rotation", contexte.temps_courant);
	auto taille = evalue_vecteur("taille", contexte.temps_courant);

	m_objet.transformation = math::construit_transformation(position, rotation, taille);
#endif
}

/* ************************************************************************** */

static void evalue_composite(Jorjala &jorjala, Composite *composite)
{
    auto &graphe = composite->noeud->graphe;

    /* Essaie de trouver une visionneuse. */

    Noeud *visionneuse = graphe.dernier_noeud_sortie;

    if (visionneuse == nullptr) {
        for (auto noeud : graphe.noeuds()) {
            if (noeud->sorties.est_vide()) {
                visionneuse = noeud;
                break;
            }
        }
    }

    /* Quitte si aucune visionneuse. */
    if (visionneuse == nullptr) {
        return;
    }

    auto contexte = cree_contexte_evaluation(jorjala);
    execute_noeud(*visionneuse, contexte, nullptr);

    Image image;
    auto operatrice = extrait_opimage(visionneuse->donnees);
    operatrice->transfere_image(image);
    composite->image(image);
}

/* ************************************************************************** */

static void execute_plan_ex(Jorjala &jorjala,
                            ContexteEvaluation const &contexte,
                            Planifieuse::PtrPlan const &plan)
{
    for (auto &noeud_res : plan->noeuds) {
        auto noeud = noeud_res->noeud;

        if (noeud != nullptr) {
            DEBUT_LOG_EVALUATION << "Évaluation de : " << noeud->nom << FIN_LOG_EVALUATION;
        }

        if (noeud->type == type_noeud::OBJET) {
            evalue_objet(contexte, extrait_objet(noeud->donnees));
        }
        else if (noeud->type == type_noeud::COMPOSITE) {
            evalue_composite(jorjala, extrait_composite(noeud->donnees));
        }
    }
}

/* ************************************************************************** */

class TacheEvaluationPlan : public TacheJorjala {
    Planifieuse::PtrPlan m_plan;
    ContexteEvaluation m_contexte;

  public:
    explicit TacheEvaluationPlan(Jorjala &jorjala,
                                 Planifieuse::PtrPlan plan,
                                 ContexteEvaluation const &contexte);

    TacheEvaluationPlan(TacheEvaluationPlan const &) = default;
    TacheEvaluationPlan &operator=(TacheEvaluationPlan const &) = default;

    void evalue() override;
};

TacheEvaluationPlan::TacheEvaluationPlan(Jorjala &jorjala,
                                         Planifieuse::PtrPlan plan,
                                         ContexteEvaluation const &contexte)
    : TacheJorjala(jorjala), m_plan(plan), m_contexte(contexte)
{
}

void TacheEvaluationPlan::evalue()
{
    DEBUT_LOG_EVALUATION << "------------------------------------" << FIN_LOG_EVALUATION;
    DEBUT_LOG_EVALUATION << "Le plan a " << m_plan->noeuds.taille() << " noeuds"
                         << FIN_LOG_EVALUATION;

    execute_plan_ex(m_jorjala, m_contexte, m_plan);

    notifier.signalise_proces(type_evenement::objet | type_evenement::traite);
    notifier.signalise_proces(type_evenement::image | type_evenement::traite);
}

/* ************************************************************************** */

void Executrice::execute_plan(Jorjala &jorjala,
                              const Planifieuse::PtrPlan &plan,
                              ContexteEvaluation const &contexte)
{
    /* si le plan peut-être lancé dans son thread (e.g. ce n'est pas pour
     * une animation), lance-y le.
     */

    /* nous sommes déjà dans un thread */
    if (plan->est_animation) {
        DEBUT_LOG_EVALUATION << "Évaluation animation pour '" << plan->message << "' ..."
                             << FIN_LOG_EVALUATION;

        execute_plan_ex(jorjala, contexte, plan);
        jorjala.tache_en_cours = false;
        return;
    }

    /* nous avons un objet simple, lance un thread */
    DEBUT_LOG_EVALUATION << "Évaluation objet asynchrone pour '" << plan->message << "' ..."
                         << FIN_LOG_EVALUATION;

    auto t = new (tbb::task::allocate_root()) TacheEvaluationPlan(jorjala, plan, contexte);
    tbb::task::enqueue(*t);
}
