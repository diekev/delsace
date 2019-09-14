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

#include "rendu.hh"

#include "biblinternes/outils/fichier.hh"

#include "danjo/danjo.h"

#include "base_de_donnees.hh"
#include "mikisa.h"
#include "noeud.hh"
#include "noeud_image.h"
#include "operatrice_image.h"

Rendu::Rendu(Noeud &n)
	: noeud(n)
{
	noeud.peut_avoir_graphe = true;
	noeud.donnees = this;
	noeud.graphe.type = type_graphe::RENDU;
}

/* ************************************************************************** */

Rendu *cree_rendu_defaut(Mikisa &mikisa)
{
	auto rendu = mikisa.bdd.cree_rendu("rendu");

	auto &graphe = rendu->noeud.graphe;

	auto noeud_sortie = graphe.cree_noeud("Moteur Rendu", type_noeud::OPERATRICE);
	auto op = mikisa.usine_operatrices()("Moteur Rendu", graphe, *noeud_sortie);
	synchronise_donnees_operatrice(*noeud_sortie);
	danjo::initialise_entreface(op, dls::contenu_fichier(op->chemin_entreface()).c_str());

	auto noeud_objets = graphe.cree_noeud("Cherche Objets", type_noeud::OPERATRICE);
	op = mikisa.usine_operatrices()("Cherche Objets", graphe, *noeud_objets);
	synchronise_donnees_operatrice(*noeud_objets);
	danjo::initialise_entreface(op, dls::contenu_fichier(op->chemin_entreface()).c_str());

	graphe.connecte(noeud_objets->sortie(0), noeud_sortie->entree(0));

	return rendu;
}

void evalue_graphe_rendu(Rendu *rendu, ContexteEvaluation &contexte)
{
	if (rendu == nullptr) {
		return;
	}

	auto noeud_sortie = static_cast<Noeud *>(nullptr);

	for (auto noeud : rendu->noeud.graphe.noeuds()) {
		if (noeud->est_sortie) {
			noeud_sortie = noeud;
			break;
		}
	}

	if (noeud_sortie == nullptr) {
		return;
	}

	auto op = extrait_opimage(noeud_sortie->donnees);
	op->execute(contexte, nullptr);
}
