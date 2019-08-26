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

#include "contexte_evaluation.hh"

#include "chef_execution.hh"
#include "configuration.h"
#include "mikisa.h"

ContexteEvaluation cree_contexte_evaluation(const Mikisa &mikisa)
{
	auto rectangle = Rectangle{};
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
	contexte.gestionnaire_fichier = const_cast<GestionnaireFichier *>(&mikisa.gestionnaire_fichier);
	contexte.chef = const_cast<ChefExecution *>(&mikisa.chef_execution);
	contexte.resolution_rendu = rectangle;
	contexte.lcc = mikisa.lcc;

	contexte.chef->reinitialise();

	return contexte;
}
