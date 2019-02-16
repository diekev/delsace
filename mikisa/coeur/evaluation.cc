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
#include "evenement.h"
#include "mikisa.h"
#include "noeud_image.h"
#include "operatrice_image.h"
#include "operatrice_objet.h"
#include "operatrice_scene.h"
#include "tache.h"

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
	evalue();
	m_mikisa.tache_en_cours = false;
	return nullptr;
}

/* ************************************************************************** */

void evalue_resultat(Mikisa &mikisa)
{
	switch (mikisa.contexte) {
		case GRAPHE_SCENE:
			evalue_scene(mikisa);
			break;
		case GRAPHE_SIMULATION:
		case GRAPHE_MAILLAGE:
		case GRAPHE_OBJET:
			evalue_objet(mikisa);
			break;
		case GRAPHE_PIXEL:
		case GRAPHE_COMPOSITE:
		default:
			evalue_graphe(mikisa);
			break;
	}
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
	auto &composite = m_mikisa.composite;
	auto &graphe = composite->graph();

	/* Essaie de trouver une visionneuse. */

	Noeud *visionneuse = m_mikisa.derniere_visionneuse_selectionnee;

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
	rectangle.hauteur = static_cast<float>(m_mikisa.project_settings->hauteur);
	rectangle.largeur = static_cast<float>(m_mikisa.project_settings->largeur);

	execute_noeud(visionneuse, rectangle, m_mikisa.temps_courant);

	Image image;
	auto operatrice = std::any_cast<OperatriceImage *>(visionneuse->donnees());
	operatrice->transfere_image(image);
	composite->image(image);
	image.reinitialise(true);

	notifier.signalImageProcessed();
}

void evalue_graphe(Mikisa &mikisa)
{
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto t = new(tbb::task::allocate_root()) GraphEvalTask(mikisa);
	tbb::task::enqueue(*t);
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

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();
	operatrice->execute(rectangle, mikisa.temps_courant);

	auto const t1 = tbb::tick_count::now();
	auto const delta = (t1 - t0).seconds();
	noeud->temps_execution(static_cast<float>(delta));

	return operatrice->objet();
}

/* ************************************************************************** */

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
	auto noeud = m_mikisa.derniere_scene_selectionnee;

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
	rectangle.hauteur = static_cast<float>(m_mikisa.project_settings->hauteur);
	rectangle.largeur = static_cast<float>(m_mikisa.project_settings->largeur);

	auto scene = operatrice->scene();
	scene->reinitialise();

	auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);
	auto graphe = operatrice_scene->graphe();

	for (auto &noeud_graphe : graphe->noeuds()) {
		auto objet = evalue_objet_ex(m_mikisa, noeud_graphe.get());

		if (objet != nullptr) {
			scene->ajoute_objet(objet);
		}
	}

	m_mikisa.notifie_observatrices(type_evenement::scene | type_evenement::traite);
}

void evalue_scene(Mikisa &mikisa)
{
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto t = new(tbb::task::allocate_root()) TacheEvaluationScene(mikisa);
	tbb::task::enqueue(*t);
}

/* ************************************************************************** */

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
	/* l'objet courant doit être le noeud actif du graphe de la dernière scène
		 * sélectionnée */
	auto noeud = m_mikisa.derniere_scene_selectionnee;

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
		m_mikisa.tache_en_cours = false;
		return;
	}

	evalue_objet_ex(m_mikisa, noeud);

	m_mikisa.notifie_observatrices(type_evenement::objet | type_evenement::traite);
}

void evalue_objet(Mikisa &mikisa)
{
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto t = new(tbb::task::allocate_root()) TacheEvaluationObjet(mikisa);
	tbb::task::enqueue(*t);
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
