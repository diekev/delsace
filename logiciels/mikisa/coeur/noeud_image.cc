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

#include "biblinternes/graphe/noeud.h"

#include "operatrice_image.h"

void execute_noeud(
		Noeud *noeud,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	auto operatrice = extrait_opimage(noeud->donnees());

	if (!noeud->besoin_execution() && !operatrice->execute_toujours()) {
		return;
	}

	noeud->temps_execution(0.0f);

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();

	auto const resultat = operatrice->execute(contexte, donnees_aval);

	/* Ne prend en compte que le temps des exécutions réussies pour éviter de se
	 * retrouver avec un temps d'exécution minimum trop bas, proche de zéro, en
	 * cas d'avortement prématuré de l'exécution. */
	if (resultat == EXECUTION_REUSSIE) {
		auto const t1 = tbb::tick_count::now();
		auto const delta = (t1 - t0).seconds();

		auto temps_parent = 0.0f;

		for (auto entree : noeud->entrees()) {
			if (entree->liens.est_vide()) {
				continue;
			}

			temps_parent += entree->liens[0]->parent->temps_execution();
		}

		noeud->incremente_compte_execution();
		noeud->temps_execution(static_cast<float>(delta) - temps_parent);
		noeud->besoin_execution(false);
	}
}

void synchronise_donnees_operatrice(Noeud *noeud)
{
	auto op = extrait_opimage(noeud->donnees());

	for (auto i = 0; i < op->entrees(); ++i) {
		noeud->ajoute_entree(op->nom_entree(i), op->type_entree(i));
	}

	for (auto i = 0; i < op->sorties(); ++i) {
		noeud->ajoute_sortie(op->nom_sortie(i), op->type_sortie(i));
	}

	auto index = 0l;

	for (auto entree : noeud->entrees()) {
		op->donnees_entree(index++, entree);
	}

	index = 0ul;

	for (auto sortie : noeud->sorties()) {
		op->donnees_sortie(index++, sortie);
	}
}
