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

#include "plan.hh"

#include <algorithm>

#include "reseau.hh"

/* ************************************************************************** */

template <typename I, typename P>
static void tri_topologique(I debut, I fin, P predicat)
{
	while (debut != fin) {
		auto nouveau_debut = std::partition(debut, fin, predicat);

		/* Aucune solution n'a été trouvée, il est possible qu'il y ait
		 * un cycle dans le graphe. */
		if (nouveau_debut == debut) {
			break;
		}

		debut = nouveau_debut;
	}
}

static void tri_graphe_plan(Planifieuse::PtrPlan plan, NoeudReseau *noeud_temps)
{
	auto predicat = [](NoeudReseau *noeud)
	{
		if (noeud->degree != 0) {
			return false;
		}

		/* Diminue le degrée des noeuds attachés à celui-ci. */
		for (auto enfant : noeud->sorties) {
			enfant->degree -= 1;
		}

		return true;
	};

	/* réduit le degrée des noeuds connectés au noeud temps */
	if (noeud_temps != nullptr) {
		predicat(noeud_temps);
	}

	tri_topologique(plan->noeuds.debut(), plan->noeuds.fin(), predicat);
}

/* ************************************************************************** */

static void rassemble_noeuds(
		dls::tableau<NoeudReseau *> &noeuds,
		std::set<NoeudReseau *> &noeuds_visites,
		NoeudReseau *noeud)
{
	if (noeuds_visites.find(noeud) != noeuds_visites.end()) {
		return;
	}

	noeuds.pousse(noeud);
	noeuds_visites.insert(noeud);

	for (auto enfant : noeud->sorties) {
		rassemble_noeuds(noeuds, noeuds_visites, enfant);
	}
}

/* ************************************************************************** */

Planifieuse::PtrPlan Planifieuse::requiers_plan_pour_scene(Reseau &reseau) const
{
	auto plan = std::make_shared<Plan>();
	plan->noeuds = reseau.noeuds;

	/* Prépare les noeuds au tri topologique. */
	for (auto noeud_dep : plan->noeuds) {
		noeud_dep->degree = static_cast<int>(noeud_dep->entrees.size());
	}

	tri_graphe_plan(plan, &reseau.noeud_temps);

	return plan;
}

Planifieuse::PtrPlan Planifieuse::requiers_plan_pour_objet(Reseau &reseau, Objet *objet) const
{
	auto plan = std::make_shared<Plan>();

	std::set<NoeudReseau *> noeuds_visites;

	for (auto noeud : reseau.noeuds) {
		if (noeud->objet == objet) {
			rassemble_noeuds(plan->noeuds, noeuds_visites, noeud);
			break;
		}
	}

	/* Prépare les noeuds au tri topologique, le degré doit être égal au nombre
	 * de noeuds parents dans la branche. */
	for (auto noeud_dep : plan->noeuds) {
		for (auto noeud_aval : noeud_dep->entrees) {
			if (noeuds_visites.find(noeud_aval) != noeuds_visites.end()) {
				noeud_dep->degree += 1;
			}
		}
	}

	tri_graphe_plan(plan, nullptr);

	return plan;
}

Planifieuse::PtrPlan Planifieuse::requiers_plan_pour_nouveau_temps(Reseau &reseau, int temps, bool est_animation) const
{
	auto plan = std::make_shared<Plan>();
	plan->temps = temps;
	plan->est_animation = est_animation;
	plan->est_pour_temps = true;

	std::set<NoeudReseau *> noeuds_visites;

	/* À FAIRE : le noeud temps ne doit pas être dans le plan. */
	rassemble_noeuds(plan->noeuds, noeuds_visites, &reseau.noeud_temps);

	/* Prépare les noeuds au tri topologique, le degré doit être égal au nombre
	 * de noeuds parents dans la branche. */
	for (auto noeud_dep : plan->noeuds) {
		for (auto noeud_aval : noeud_dep->entrees) {
			if (noeuds_visites.find(noeud_aval) != noeuds_visites.end()) {
				noeud_dep->degree += 1;
			}
		}
	}

	tri_graphe_plan(plan, &reseau.noeud_temps);

	return plan;
}
