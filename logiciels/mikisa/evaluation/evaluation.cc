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

#include "evaluation.hh"

#include <iostream>

#include "biblinternes/structures/pile.hh"

#include "coeur/composite.h"
#include "coeur/configuration.h"
#include "coeur/contexte_evaluation.hh"
#include "coeur/mikisa.h"
#include "coeur/objet.h"
#include "coeur/operatrice_image.h"
#include "coeur/operatrice_graphe_detail.hh"

#include "execution.hh"
#include "reseau.hh"

/* ************************************************************************** */

static void insere_noeud(
		dls::pile<Noeud *> &noeuds,
		dls::ensemble<Noeud *> &noeuds_visites,
		Noeud *noeud)
{
	if (noeuds_visites.trouve(noeud) == noeuds_visites.fin()) {
		noeuds.empile(noeud);
		noeuds_visites.insere(noeud);
	}
}

static void rassemble_noeuds_chronodependants(
		Graphe &graphe,
		dls::pile<Noeud *> &noeuds,
		dls::ensemble<Noeud *> &noeuds_visites)
{
	for (auto &noeud : graphe.noeuds()) {
		auto op = extrait_opimage(noeud->donnees);

		if (noeud->peut_avoir_graphe) {
			rassemble_noeuds_chronodependants(noeud->graphe, noeuds, noeuds_visites);
		}

		if (op->depend_sur_temps()) {
			insere_noeud(noeuds, noeuds_visites, noeud);

			auto p = noeud->parent;

			while (p != nullptr) {
				insere_noeud(noeuds, noeuds_visites, p);
				p = p->parent;
			}
		}
	}
}

static void notifie_noeuds_chronodependants(Graphe &graphe)
{
	auto pile = dls::pile<Noeud *>();
	auto visites = dls::ensemble<Noeud *>();

	rassemble_noeuds_chronodependants(graphe, pile, visites);

	while (!pile.est_vide()) {
		auto noeud = pile.depile();

		noeud->besoin_execution = true;

		for (auto prise : noeud->sorties) {
			for (auto lien : prise->liens) {
				pile.empile(lien->parent);
			}
		}
	}
}

void requiers_evaluation(Mikisa &mikisa, int raison, const char *message)
{
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto evalue_composite = (mikisa.graphe->type == type_graphe::COMPOSITE);

	if (mikisa.graphe->type == type_graphe::DETAIL) {
		auto graphe_detail = mikisa.graphe;
		auto type_detail = std::any_cast<int>(graphe_detail->donnees[0]);
		evalue_composite = (type_detail == DETAIL_PIXELS);
	}

	if (evalue_composite) {
		auto noeud_actif = mikisa.bdd.graphe_composites()->noeud_actif;
		auto composite = extrait_composite(noeud_actif->donnees);

		if (raison == TEMPS_CHANGE) {
			notifie_noeuds_chronodependants(composite->noeud->graphe);
		}

		execute_graphe_composite(mikisa, composite, message);
		return;
	}

	auto planifieuse = Planifieuse{};
	auto executrice = Executrice{};

	auto compileuse = CompilatriceReseau{};
	auto &reseau = mikisa.reseau;
	compileuse.reseau = &reseau;

	auto objet = static_cast<Objet *>(nullptr);
	auto noeud_actif = mikisa.bdd.graphe_objets()->noeud_actif;

	if (noeud_actif != nullptr) {
		objet = extrait_objet(noeud_actif->donnees);
	}

	auto contexte = cree_contexte_evaluation(mikisa);
	compileuse.compile_reseau(contexte, &mikisa.bdd, objet);

	auto plan = Planifieuse::PtrPlan{nullptr};

	switch (raison) {
		case NOEUD_AJOUTE:
		case NOEUD_ENLEVE:
		case NOEUD_SELECTIONE:
		case GRAPHE_MODIFIE:
		case PARAMETRE_CHANGE:
		{
			plan = planifieuse.requiers_plan_pour_objet(reseau, objet);
			break;
		}
		case OBJET_AJOUTE:
		case OBJET_ENLEVE:
		case FICHIER_OUVERT:
		case RENDU_REQUIS:
		{
			plan = planifieuse.requiers_plan_pour_scene(reseau);
			break;
		}
		case TEMPS_CHANGE:
		{
			compileuse.marque_execution_temps_change();
			plan = planifieuse.requiers_plan_pour_nouveau_temps(reseau, mikisa.temps_courant, mikisa.animation);
			break;
		}
	}

	plan->message = message;

	executrice.execute_plan(mikisa, plan, contexte);
}
