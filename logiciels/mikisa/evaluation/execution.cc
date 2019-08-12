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
#include "coeur/mikisa.h"
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

#define DEBUT_LOG_EVALUATION if (GV_log_evaluation) { std::cerr
#define FIN_LOG_EVALUATION  '\n'; }

/* À FAIRE, évaluation asynchrone :
 * - TBB semblerait avoir du mal avec les tâches trop rapides, il arrive qu'au
 *   bout d'un moment les tâches ne sont plus exécutées, car les threads n'ont
 *   pas eu le temps de se joindre.
 * - si une tâche est en cours, ajout des nouvelles tâches dans une queue et
 *   retourne la première de celles-ci dans tbb::task::execute().
 * - une tâche est lancée au démarrage, pourquoi ?
 */

/* ************************************************************************** */

class TacheMikisa : public tbb::task {
protected:
	TaskNotifier notifier;
	Mikisa &m_mikisa;

public:
	explicit TacheMikisa(Mikisa &mikisa);

	virtual void evalue() = 0;

	tbb::task *execute() override;
};

TacheMikisa::TacheMikisa(Mikisa &mikisa)
	: notifier(mikisa.fenetre_principale)
	, m_mikisa(mikisa)
{}

tbb::task *TacheMikisa::execute()
{
	m_mikisa.interrompu = false;
	notifier.signale_debut_tache();
	evalue();
	notifier.signale_fin_tache();

	m_mikisa.tache_en_cours = false;
	return nullptr;
}

/* ************************************************************************** */

static void evalue_objet(ContexteEvaluation const &contexte, Objet *objet)
{
	if (objet == nullptr) {
		std::cerr << "ERREUR : l'objet est nul !\n";
		return;
	}

	auto &graphe = objet->graphe;
	auto noeud_sortie = graphe.dernier_noeud_sortie;

	if (noeud_sortie == nullptr) {
		return;
	}

	auto operatrice = extrait_opimage(noeud_sortie->donnees());

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();
	operatrice->execute(contexte, nullptr);

	auto const t1 = tbb::tick_count::now();
	auto const delta = (t1 - t0).seconds();
	noeud_sortie->temps_execution(static_cast<float>(delta));

	/* À FAIRE? :- on garde une copie pour l'évaluation dans des threads
	 * séparés, copie nécessaire pour pouvoir rendre l'objet dans la vue quand
	 * le rendu prend plus de temps que l'évaluation asynchrone. */
	objet->donnees.accede_ecriture([operatrice](DonneesObjet *donnees_objet)
	{
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

class TacheEvaluationPlan : public TacheMikisa {
	Planifieuse::PtrPlan m_plan;
	ContexteEvaluation m_contexte;

public:
	explicit TacheEvaluationPlan(
			Mikisa &mikisa,
			Planifieuse::PtrPlan plan,
			ContexteEvaluation const &contexte);

	TacheEvaluationPlan(TacheEvaluationPlan const &) = default;
	TacheEvaluationPlan &operator=(TacheEvaluationPlan const &) = default;

	void evalue() override;
};

TacheEvaluationPlan::TacheEvaluationPlan(
		Mikisa &mikisa,
		Planifieuse::PtrPlan plan,
		ContexteEvaluation const &contexte)
	: TacheMikisa(mikisa)
	, m_plan(plan)
	, m_contexte(contexte)
{}

void TacheEvaluationPlan::evalue()
{
	DEBUT_LOG_EVALUATION << "------------------------------------" << FIN_LOG_EVALUATION;
	DEBUT_LOG_EVALUATION << "Le plan a "
						 << m_plan->noeuds.taille()
						 << " noeuds"
						 << FIN_LOG_EVALUATION;

	for (auto &noeud : m_plan->noeuds) {
		if (noeud->objet != nullptr) {
			DEBUT_LOG_EVALUATION << "Évaluation de : " << noeud->objet->nom << FIN_LOG_EVALUATION;
		}

		evalue_objet(m_contexte, noeud->objet);
	}

	notifier.signalise_proces(type_evenement::objet | type_evenement::traite);
}

/* ************************************************************************** */

void Executrice::execute_plan(Mikisa &mikisa,
		const Planifieuse::PtrPlan &plan,
		ContexteEvaluation const &contexte)
{
	/* si le plan peut-être lancé dans son thread (e.g. ce n'est pas pour
	 * une animation), lance-y le.
	 */

	/* nous sommes déjà dans un thread */
	if (plan->est_animation) {
		DEBUT_LOG_EVALUATION << "Évaluation animation pour '"
							 << plan->message
							 << "' ..."
							 << FIN_LOG_EVALUATION;

		/* tag les noeuds des graphes pour l'exécution temporelle */
		for (auto &noeud : plan->noeuds) {
			if (noeud->objet != nullptr) {
				DEBUT_LOG_EVALUATION << "Évaluation de : " << noeud->objet->nom << FIN_LOG_EVALUATION;
			}

			evalue_objet(contexte, noeud->objet);
		}

		mikisa.tache_en_cours = false;
		return;
	}

	/* nous avons un objet simple, lance un thread */
	DEBUT_LOG_EVALUATION << "Évaluation objet asynchrone pour '"
						 << plan->message
						 << "' ..."
						 << FIN_LOG_EVALUATION;

	auto t = new(tbb::task::allocate_root()) TacheEvaluationPlan(mikisa, plan, contexte);
	tbb::task::enqueue(*t);
}

/* ************************************************************************** */

class TacheEvaluationComposite : public TacheMikisa {
	Composite *m_composite;

public:
	explicit TacheEvaluationComposite(
			Mikisa &mikisa,
			Composite *composite);

	TacheEvaluationComposite(TacheEvaluationComposite const &) = default;
	TacheEvaluationComposite &operator=(TacheEvaluationComposite const &) = default;

	void evalue() override;
};

TacheEvaluationComposite::TacheEvaluationComposite(
		Mikisa &mikisa,
		Composite *composite)
	: TacheMikisa(mikisa)
	, m_composite(composite)
{}

void TacheEvaluationComposite::evalue()
{
	auto &graphe = m_composite->graph();

	/* Essaie de trouver une visionneuse. */

	Noeud *visionneuse = m_mikisa.derniere_visionneuse_selectionnee;

	if (visionneuse == nullptr) {
		for (auto noeud : graphe.noeuds()) {
			if (noeud->type() == NOEUD_IMAGE_SORTIE) {
				visionneuse = noeud;
				break;
			}
		}
	}

	/* Quitte si aucune visionneuse. */
	if (visionneuse == nullptr) {
		return;
	}

	auto contexte = cree_contexte_evaluation(m_mikisa);
	execute_noeud(visionneuse, contexte, nullptr);

	Image image;
	auto operatrice = extrait_opimage(visionneuse->donnees());
	operatrice->transfere_image(image);
	m_composite->image(image);

	notifier.signalise_proces(type_evenement::image | type_evenement::traite);
}

void execute_graphe_composite(Mikisa &mikisa, Composite *composite, const char *message)
{
	/* nous avons un objet simple, lance un thread */
	DEBUT_LOG_EVALUATION << "Évaluation asynchrone composite pour '"
						 << message
						 << "' ..."
						 << FIN_LOG_EVALUATION;

	auto t = new(tbb::task::allocate_root()) TacheEvaluationComposite(mikisa, composite);
	tbb::task::enqueue(*t);
}
