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

#include "../contexte_evaluation.hh"
#include "../evenement.h"
#include "../mikisa.h"
#include "../objet.h"
#include "../operatrice_image.h"
#include "../tache.h"

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

#if 0 /* À FAIRE : évaluation des composites. */
static void evalue_composite(Mikisa &mikisa)
{
	auto &composite = mikisa.composite;
	auto &graphe = composite->graph();

	/* Essaie de trouver une visionneuse. */

	Noeud *visionneuse = mikisa.derniere_visionneuse_selectionnee;

	if (visionneuse == nullptr) {
		for (std::shared_ptr<Noeud> const &node : graphe.noeuds()) {
			Noeud *pointeur_noeud = node.get();

			if (pointeur_noeud->type() == NOEUD_IMAGE_SORTIE) {
				visionneuse = pointeur_noeud;
				break;
			}
		}
	}

	/* Quitte si aucune visionneuse. */
	if (visionneuse == nullptr) {
		return;
	}

	Rectangle rectangle;
	rectangle.x = 0;
	rectangle.y = 0;
	rectangle.hauteur = static_cast<float>(mikisa.project_settings->hauteur);
	rectangle.largeur = static_cast<float>(mikisa.project_settings->largeur);

	auto contexte = ContexteEvaluation{};
	contexte.bdd = &mikisa.bdd;
	contexte.cadence = mikisa.cadence;
	contexte.temps_debut = mikisa.temps_debut;
	contexte.temps_fin = mikisa.temps_fin;
	contexte.temps_courant = mikisa.temps_courant;
	contexte.resolution_rendu = rectangle;
	contexte.gestionnaire_fichier = &mikisa.gestionnaire_fichier;
	contexte.chef = &mikisa.chef_execution;

	execute_noeud(visionneuse, contexte, nullptr);

	Image image;
	auto operatrice = std::any_cast<OperatriceImage *>(visionneuse->donnees());
	operatrice->transfere_image(image);
	composite->image(image);
	image.reinitialise(true);
}

class GraphEvalTask : public TacheMikisa {
public:
	explicit GraphEvalTask(Mikisa &mikisa);

	GraphEvalTask(GraphEvalTask const &) = default;
	GraphEvalTask &operator=(GraphEvalTask const &) = default;

	void evalue() override;
};

GraphEvalTask::GraphEvalTask(Mikisa &mikisa)
	: TacheMikisa(mikisa)
{}

void GraphEvalTask::evalue()
{
	evalue_composite(m_mikisa);
	notifier.signalImageProcessed();
}

void evalue_graphe(Mikisa &mikisa, const char *message)
{
	DEBUT_LOG_EVALUATION << "Évaluation composite pour '"
						 << message
						 << "' ..."
						 << FIN_LOG_EVALUATION;

#if 0
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto t = new(tbb::task::allocate_root()) GraphEvalTask(mikisa);
	tbb::task::enqueue(*t);
#else
	/* À FAIRE : le rendu OpenGL pour les noeuds scènes ne peut se faire dans un
	 * thread séparé... */
	evalue_composite(mikisa);
	mikisa.notifie_observatrices(type_evenement::image | type_evenement::traite);
#endif
}
#endif

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

	auto operatrice = std::any_cast<OperatriceImage *>(noeud_sortie->donnees());

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();
	operatrice->execute(contexte, nullptr);

	auto const t1 = tbb::tick_count::now();
	auto const delta = (t1 - t0).seconds();
	noeud_sortie->temps_execution(static_cast<float>(delta));

	/* À FAIRE? :- on garde une copie pour l'évaluation dans des threads
	 * séparés, copie nécessaire pour pouvoir rendre l'objet dans la vue quand
	 * le rendu prend plus de temps que l'évaluation asynchrone. */
	objet->corps.accede_ecriture([operatrice](Corps &_corps_)
	{
		_corps_.reinitialise();
		operatrice->corps()->copie_vers(&_corps_);
	});

#if 0 /* À FAIRE : interface pour la transformation */
	auto position = dls::math::point3f(evalue_vecteur("position", contexte.temps_courant));
	auto rotation = evalue_vecteur("rotation", contexte.temps_courant);
	auto taille = evalue_vecteur("taille", contexte.temps_courant);

	auto transformation = math::transformation();
	transformation *= math::translation(position.x, position.y, position.z);
	transformation *= math::rotation_x(rotation.x * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_y(rotation.y * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_z(rotation.z * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::echelle(taille.x, taille.y, taille.z);

	m_objet.transformation = transformation;
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
		DEBUT_LOG_EVALUATION << "Évaluation de : " << noeud->objet->nom << FIN_LOG_EVALUATION;
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
		/* tag les noeuds des graphes pour l'exécution temporelle */
		for (auto &noeud : plan->noeuds) {
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
