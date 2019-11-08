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

#include "outils.hh"

#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"

#include "coeur/noeud.hh"
#include "coeur/operatrice_image.h"

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

			while (p != nullptr && p->type == type_noeud::OPERATRICE) {
				insere_noeud(noeuds, noeuds_visites, p);
				p = p->parent;
			}
		}
	}
}

void notifie_noeuds_chronodependants(Graphe &graphe)
{
	auto pile = dls::pile<Noeud *>();
	auto visites = dls::ensemble<Noeud *>();

	rassemble_noeuds_chronodependants(graphe, pile, visites);

	while (!pile.est_vide()) {
		auto noeud = pile.depile();

		noeud->besoin_execution = true;
		auto op = extrait_opimage(noeud->donnees);
		op->amont_change(nullptr);

		for (auto prise : noeud->sorties) {
			for (auto lien : prise->liens) {
				pile.empile(lien->parent);
			}
		}
	}
}
