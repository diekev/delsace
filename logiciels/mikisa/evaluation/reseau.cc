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

#include "reseau.hh"

#include "biblinternes/structures/pile.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/composite.h"
#include "coeur/objet.h"
#include "coeur/operatrice_image.h"

#include "outils.hh"

/* ************************************************************************** */

static void marque_execution_graphe(NoeudReseau *racine)
{
	for (auto noeud_res : racine->sorties) {
		for (auto &noeud : noeud_res->noeud->graphe.noeuds()) {
			noeud->besoin_execution = true;
			auto op = extrait_opimage(noeud->donnees);
			op->amont_change(nullptr);
		}

		marque_execution_graphe(noeud_res);
	}
}

static void marque_execution_graphe_temps_change(NoeudReseau *racine)
{
	for (auto noeud_res : racine->sorties) {
		notifie_noeuds_chronodependants(noeud_res->noeud->graphe);
		marque_execution_graphe_temps_change(noeud_res);
	}
}

static void cree_dependances(
		Graphe &graphe,
		NoeudReseau *noeud_temps,
		NoeudReseau *noeud_dep,
		CompilatriceReseau &compilatrice,
		ContexteEvaluation &contexte)
{
	auto noeuds = dls::pile<Noeud *>();

	/* n'ajoute les dépendances que pour les noeuds connectés aux sorties */
	for (auto noeud : graphe.noeuds()) {
		if (noeud->sorties.est_vide()) {
			noeuds.empile(noeud);
		}
	}

	auto noeuds_visites = dls::ensemble<Noeud *>();

	while (!noeuds.est_vide()) {
		auto noeud = noeuds.depile();

		if (noeuds_visites.trouve(noeud) != noeuds_visites.fin()) {
			continue;
		}

		noeuds_visites.insere(noeud);

		auto operatrice = extrait_opimage(noeud->donnees);

		if (operatrice->possede_animation()) {
			compilatrice.ajoute_dependance(noeud_temps, noeud_dep);
		}
		else if (operatrice->depend_sur_temps()) {
			compilatrice.ajoute_dependance(noeud_temps, noeud_dep);
		}

		operatrice->renseigne_dependance(contexte, compilatrice, noeud_dep);

		if (noeud->peut_avoir_graphe) {
			cree_dependances(noeud->graphe, noeud_temps, noeud_dep, compilatrice, contexte);
		}

		for (auto prise_entree : noeud->entrees) {
			for (auto prise_sortie : prise_entree->liens) {
				noeuds.empile(prise_sortie->parent);
			}
		}
	}

	/* vide les tampons mémoires des noeuds déconnectés */
	for (auto noeud : graphe.noeuds()) {
		if (noeuds_visites.trouve(noeud) != noeuds_visites.fin()) {
			continue;
		}

		auto operatrice = extrait_opimage(noeud->donnees);
		operatrice->libere_memoire();
		noeud->besoin_execution = true;
	}
}

/* ************************************************************************** */

Reseau::~Reseau()
{
	reinitialise();
}

void Reseau::reinitialise()
{
	for (auto noeud : this->noeuds) {
		memoire::deloge("NoeudReseau", noeud);
	}

	noeuds.efface();
	noeud_temps.sorties.efface();
}

/* ************************************************************************** */

void CompilatriceReseau::cree_noeud(Noeud *noeud)
{
	auto iter_noeud = m_table_noeud_noeud.trouve(noeud);

	if (iter_noeud != m_table_noeud_noeud.fin()) {
		throw std::runtime_error("Un noeud existe déjà pour l'objet !");
	}

	auto noeud_reseau = memoire::loge<NoeudReseau>("NoeudReseau");
	noeud_reseau->noeud = noeud;
	reseau->noeuds.pousse(noeud_reseau);

	m_table_noeud_noeud.insere({noeud, noeud_reseau});
}

NoeudReseau *CompilatriceReseau::trouve_noeud_reseau_pour_noeud(Noeud *noeud)
{
	auto iter_noeud = m_table_noeud_noeud.trouve(noeud);

	if (iter_noeud == m_table_noeud_noeud.fin()) {
		throw std::runtime_error("Aucun noeud n'existe pour l'objet !");
	}

	return iter_noeud->second;
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud_reseau, Noeud *noeud)
{
	ajoute_dependance(trouve_noeud_reseau_pour_noeud(noeud), noeud_reseau);
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud_de, NoeudReseau *noeud_vers)
{
	noeud_de->sorties.insere(noeud_vers);
	noeud_vers->entrees.insere(noeud_de);
}

void CompilatriceReseau::compile_reseau(
		ContexteEvaluation &contexte,
		BaseDeDonnees *bdd,
		Noeud *noeud_racine)
{
	reseau->reinitialise();

	/* crée les noeuds */
	for (auto objet : bdd->objets()) {
		cree_noeud(objet->noeud);
	}

	for (auto composite : bdd->composites()) {
		cree_noeud(composite->noeud);
	}

	/* crée les dépendances */
	for (auto noeud_reseau : reseau->noeuds) {
		auto noeud = noeud_reseau->noeud;

		if (noeud->possede_animation()) {
			ajoute_dependance(&reseau->noeud_temps, noeud_reseau);
		}

		cree_dependances(noeud->graphe, &reseau->noeud_temps, noeud_reseau, *this, contexte);
	}

	/* Marque les graphes des objets en aval comm ayant besoin d'une exécution.
	 * À FAIRE : trouve les noeuds exacts à exécuter. */
	if (noeud_racine != nullptr) {
		auto noeud = trouve_noeud_reseau_pour_noeud(noeud_racine);
		marque_execution_graphe(noeud);
	}
}

void CompilatriceReseau::marque_execution_temps_change()
{
	marque_execution_graphe_temps_change(&reseau->noeud_temps);
}
