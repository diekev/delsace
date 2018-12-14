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

#include "noeud_image.h"

#include <tbb/tick_count.h>

#include "bibliotheques/graphe/noeud.h"

#include "operatrice_image.h"

void execute_noeud(Noeud *noeud, const Rectangle &rectangle, const int temps)
{
	if (!noeud->besoin_execution()) {
		return;
	}

	noeud->temps_execution(0.0f);

	auto const t0 = tbb::tick_count::now();

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
	operatrice->reinitialise_avertisements();

	auto const resultat = operatrice->execute(rectangle, temps);

	/* Ne prend en compte que le temps des exécutions réussies pour éviter de se
	 * retrouver avec un temps d'exécution minimum trop bas, proche de zéro, en
	 * cas d'avortement prématuré de l'exécution. */
	if (resultat == EXECUTION_REUSSIE) {
		auto const t1 = tbb::tick_count::now();
		auto const delta = (t1 - t0).seconds();

		auto temps_parent = 0.0f;

		for (auto entree : noeud->entrees()) {
			if (!entree->lien) {
				continue;
			}

			temps_parent += entree->lien->parent->temps_execution();
		}

		noeud->incremente_compte_execution();
		noeud->temps_execution(static_cast<float>(delta) - temps_parent);
	}
}

void synchronise_donnees_operatrice(Noeud *noeud)
{
	auto op = std::any_cast<OperatriceImage *>(noeud->donnees());

	for (size_t i = 0; i < op->inputs(); ++i) {
		noeud->ajoute_entree(op->nom_entree(static_cast<int>(i)), op->type_entree(static_cast<int>(i)));
	}

	for (size_t i = 0; i < op->outputs(); ++i) {
		noeud->ajoute_sortie(op->nom_sortie(static_cast<int>(i)), op->type_sortie(static_cast<int>(i)));
	}

	auto index = 0ul;

	for (auto entree : noeud->entrees()) {
		op->set_input_data(index++, entree);
	}

	index = 0ul;

	for (auto sortie : noeud->sorties()) {
		op->set_output_data(index++, sortie);
	}
}
