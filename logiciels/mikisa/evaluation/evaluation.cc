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
#include "outils.hh"
#include "reseau.hh"

/* ************************************************************************** */

void requiers_evaluation(Mikisa &mikisa, int raison, const char *message)
{
	if (mikisa.tache_en_cours) {
		return;
	}

	mikisa.tache_en_cours = true;

	auto planifieuse = Planifieuse{};
	auto executrice = Executrice{};

	auto compileuse = CompilatriceReseau{};
	auto &reseau = mikisa.reseau;
	compileuse.reseau = &reseau;

	auto noeud_actif = noeud_base_hierarchie(mikisa.graphe->noeud_actif);

	auto contexte = cree_contexte_evaluation(mikisa);
	compileuse.compile_reseau(contexte, &mikisa.bdd, noeud_actif);

	auto plan = Planifieuse::PtrPlan{nullptr};

	switch (raison) {
		case NOEUD_AJOUTE:
		case NOEUD_ENLEVE:
		case NOEUD_SELECTIONE:
		case GRAPHE_MODIFIE:
		case PARAMETRE_CHANGE:
		{
			plan = planifieuse.requiers_plan_pour_noeud(reseau, noeud_actif);
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
