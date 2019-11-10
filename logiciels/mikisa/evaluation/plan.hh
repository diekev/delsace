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

#pragma once

#include <memory>

#include "biblinternes/structures/tableau.hh"

struct NoeudReseau;
struct Reseau;

class Noeud;

/* ************************************************************************** */

/* Design Presto :
 * les planifieuses sont responsables de planifier les phases d'exécutions,
 * utilisés par les exécutrices pour des évaluations répétées
 *
 * les clients tiennent des plans à travers des objets opaques reçus quand ils
 * requierent des évaluations
 *
 * les planifieuses retourne une liste de noeuds à exécuter
 *
 * les plans contiennent les résultat de l'analyse statique, et peut-être
 * réutilisé durant l'évaluation sans modification -> pas de verrou nécessaire
 */

/* ************************************************************************** */

struct Planifieuse {
	struct Plan {
		dls::tableau<NoeudReseau *> noeuds{};

		const char *message = nullptr;

		int temps = 0;
		bool est_animation = false;
		bool est_pour_temps = false;
		char pad[2];
	};

	/* NOTE : utilisation d'un pointeur car les plans doivent être valides dans
	 * des threads séparés, donc on ne peut pas utiliser des plans temporaires.
	 */
	using PtrPlan = std::shared_ptr<Plan>;

	PtrPlan requiers_plan_pour_scene(Reseau &reseau) const;

	PtrPlan requiers_plan_pour_noeud(Reseau &reseau, Noeud *noeud) const;

	PtrPlan requiers_plan_pour_nouveau_temps(Reseau &reseau, int temps, bool est_animation) const;
};
