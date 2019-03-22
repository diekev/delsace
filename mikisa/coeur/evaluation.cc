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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "evaluation.h"

#include <algorithm>
#include <iostream>

#include <tbb/parallel_for.h>
#include <tbb/task.h>
#include <tbb/tick_count.h>

#include "composite.h"
#include "configuration.h"
#include "contexte_evaluation.hh"
#include "evenement.h"
#include "mikisa.h"
#include "noeud_image.h"
#include "operatrice_image.h"
#include "operatrice_objet.h"
#include "operatrice_scene.h"
#include "tache.h"

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

void evalue_resultat(Mikisa &mikisa, const char *message)
{
	switch (mikisa.contexte) {
		case GRAPHE_SCENE:
			evalue_scene(mikisa, message);
			break;
		case GRAPHE_SIMULATION:
		case GRAPHE_MAILLAGE:
		case GRAPHE_OBJET:
			evalue_objet(mikisa, message);
			break;
		case GRAPHE_PIXEL:
		case GRAPHE_COMPOSITE:
		default:
			evalue_graphe(mikisa, message);
			break;
	}
}

static void evalue_composite(Mikisa &mikisa)
{
	auto &composite = mikisa.composite;
	auto &graphe = composite->graph();

	/* Essaie de trouver une visionneuse. */

	Noeud *visionneuse = mikisa.derniere_visionneuse_selectionnee;

	if (visionneuse == nullptr) {
		for (auto pointeur_noeud : graphe.noeuds()) {
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

/* ************************************************************************** */

static Objet *evalue_objet_ex(Mikisa const &mikisa, Noeud *noeud)
{
	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

	if (operatrice->type() != OPERATRICE_OBJET) {
		return nullptr;
	}

	Rectangle rectangle;
	rectangle.x = 0;
	rectangle.y = 0;
	rectangle.hauteur = static_cast<float>(mikisa.project_settings->hauteur);
	rectangle.largeur = static_cast<float>(mikisa.project_settings->largeur);

	auto contexte = ContexteEvaluation{};
	contexte.cadence = mikisa.cadence;
	contexte.temps_debut = mikisa.temps_debut;
	contexte.temps_fin = mikisa.temps_fin;
	contexte.temps_courant = mikisa.temps_courant;
	contexte.resolution_rendu = rectangle;
	contexte.gestionnaire_fichier = const_cast<GestionnaireFichier *>(&mikisa.gestionnaire_fichier);
	contexte.chef = const_cast<ChefExecution *>(&mikisa.chef_execution);

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();
	operatrice->execute(contexte, nullptr);

	auto const t1 = tbb::tick_count::now();
	auto const delta = (t1 - t0).seconds();
	noeud->temps_execution(static_cast<float>(delta));

	return operatrice->objet();
}

/* ************************************************************************** */

static auto evalue_scene_ex(Mikisa const &mikisa)
{
	auto noeud = mikisa.derniere_scene_selectionnee;

	if (noeud == nullptr) {
		return;
	}

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

	if (operatrice->type() != OPERATRICE_SCENE) {
		return;
	}

	Rectangle rectangle;
	rectangle.x = 0;
	rectangle.y = 0;
	rectangle.hauteur = static_cast<float>(mikisa.project_settings->hauteur);
	rectangle.largeur = static_cast<float>(mikisa.project_settings->largeur);

//	auto scene = operatrice->scene();

	auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);
	auto graphe = operatrice_scene->graphe();

	for (auto &noeud_graphe : graphe->noeuds()) {
		/*auto objet = */evalue_objet_ex(mikisa, noeud_graphe);

		/* À FAIRE : base de données d'objets. */
//		if (objet != nullptr) {
//			scene->ajoute_objet(objet);
//		}
	}
}

class TacheEvaluationScene : public TacheMikisa {
public:
	explicit TacheEvaluationScene(Mikisa &mikisa);

	void evalue() override;
};

TacheEvaluationScene::TacheEvaluationScene(Mikisa &mikisa)
	: TacheMikisa(mikisa)
{}

void TacheEvaluationScene::evalue()
{
	evalue_scene_ex(m_mikisa);
	m_mikisa.notifie_observatrices(type_evenement::scene | type_evenement::traite);
}

void evalue_scene(Mikisa &mikisa, const char *message)
{
	if (mikisa.animation) {
		DEBUT_LOG_EVALUATION << "Évaluation scène synchrone pour '"
							 << message
							 << "' ..."
							 << FIN_LOG_EVALUATION;

		/* Nous sommes dans une animation, donc pas la peine de lancer une tâche
		 * asynchone. */
		evalue_scene_ex(mikisa);

		DEBUT_LOG_EVALUATION << "Fin évaluation scène synchrone...." << FIN_LOG_EVALUATION;
	}
	else {
		DEBUT_LOG_EVALUATION << "Évaluation scène asynchrone pour '"
							 << message
							 << "' ..."
							 << FIN_LOG_EVALUATION;

		if (mikisa.tache_en_cours) {
			DEBUT_LOG_EVALUATION << "--- Tâche en cours.... abandon"
								 << FIN_LOG_EVALUATION;
			return;
		}

		mikisa.tache_en_cours = true;

		auto t = new(tbb::task::allocate_root()) TacheEvaluationScene(mikisa);
		tbb::task::enqueue(*t);
	}
}

/* ************************************************************************** */

static auto evalue_objet_simple_ex(Mikisa &mikisa)
{
	/* l'objet courant doit être le noeud actif du graphe de la dernière scène
	 * sélectionnée */
	auto noeud = mikisa.derniere_scene_selectionnee;

	if (noeud == nullptr) {
		return;
	}

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

	if (operatrice->type() != OPERATRICE_SCENE) {
		return;
	}

	auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);
	auto graphe = operatrice_scene->graphe();

	/* cherche le noeud objet */
	noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	evalue_objet_ex(mikisa, noeud);
}

class TacheEvaluationObjet : public TacheMikisa {
public:
	explicit TacheEvaluationObjet(Mikisa &mikisa);

	void evalue() override;
};

TacheEvaluationObjet::TacheEvaluationObjet(Mikisa &mikisa)
	: TacheMikisa(mikisa)
{}

void TacheEvaluationObjet::evalue()
{
	evalue_objet_simple_ex(m_mikisa);
	m_mikisa.notifie_observatrices(type_evenement::objet | type_evenement::traite);
}

void evalue_objet(Mikisa &mikisa, const char *message)
{
	if (mikisa.animation) {
		DEBUT_LOG_EVALUATION << "Évaluation objet synchrone pour '"
							 << message
							 << "' ..."
							 << FIN_LOG_EVALUATION;

		/* Nous sommes dans une animation, donc pas la peine de lancer une tâche
		 * asynchone. */
		evalue_objet_simple_ex(mikisa);

		DEBUT_LOG_EVALUATION << "Fin évaluation objet synchrone...."
							 << FIN_LOG_EVALUATION;
	}
	else {
		DEBUT_LOG_EVALUATION << "Évaluation objet asynchrone pour '"
							 << message
							 << "' ..."
							 << FIN_LOG_EVALUATION;

		if (mikisa.tache_en_cours) {
			DEBUT_LOG_EVALUATION << "--- Tâche en cours.... abandon"
								 << FIN_LOG_EVALUATION;
			return;
		}

		mikisa.tache_en_cours = true;

		auto t = new(tbb::task::allocate_root()) TacheEvaluationObjet(mikisa);
		tbb::task::enqueue(*t);
	}
}

/* ************************************************************************** */

#if 0
class InterruptriceTache {
	bool est_arretee = false;

public:
	bool a_ete_arretee() const
	{
		return est_arretee;
	}

	void arrete()
	{
		est_arretee = true;
	}
};
#endif
