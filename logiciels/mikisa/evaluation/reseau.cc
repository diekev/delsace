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
#include "coeur/objet.h"
#include "coeur/operatrice_image.h"

/* ************************************************************************** */

static void marque_execution_graphe(NoeudReseau *racine)
{
	for (auto noeud_res : racine->sorties) {
		auto objet = noeud_res->objet;
		auto &graphe = objet->graphe;

		for (auto &noeud : graphe.noeuds()) {
			noeud->besoin_execution(true);
			auto op = extrait_opimage(noeud->donnees());
			op->amont_change(nullptr);
		}

		marque_execution_graphe(noeud_res);
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

void CompilatriceReseau::cree_noeud(Objet *objet, Noeud *noeud_objet)
{
	auto iter_noeud = m_table_objet_noeud.trouve(objet);

	if (iter_noeud != m_table_objet_noeud.fin()) {
		throw std::runtime_error("Un noeud existe déjà pour l'objet !");
	}

	auto noeud = memoire::loge<NoeudReseau>("NoeudReseau");
	noeud->objet = objet;
	noeud->noeud_objet = noeud_objet;
	reseau->noeuds.pousse(noeud);

	m_table_objet_noeud.insere({objet, noeud});
}

NoeudReseau *CompilatriceReseau::trouve_noeud_pour_objet(Objet *objet)
{
	auto iter_noeud = m_table_objet_noeud.trouve(objet);

	if (iter_noeud == m_table_objet_noeud.fin()) {
		throw std::runtime_error("Aucun noeud n'existe pour l'objet !");
	}

	return iter_noeud->second;
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud, Objet *objet)
{
	ajoute_dependance(trouve_noeud_pour_objet(objet), noeud);
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud_de, NoeudReseau *noeud_vers)
{
	noeud_de->sorties.insere(noeud_vers);
	noeud_vers->entrees.insere(noeud_de);
}

void CompilatriceReseau::compile_reseau(ContexteEvaluation &contexte, BaseDeDonnees *bdd, Objet *objet)
{
	reseau->reinitialise();

	/* crée les noeuds */
	for (auto paire : bdd->table_objets()) {
		cree_noeud(paire.first, paire.second);
	}

	/* crée les dépendances */
	for (auto noeud_dep : reseau->noeuds) {
		auto objet_noeud = noeud_dep->objet;

		if (objet_noeud->possede_animation()) {
			ajoute_dependance(&reseau->noeud_temps, noeud_dep);
		}

		/* n'ajoute les dépendances que pour les noeuds connectés à la sortie */
		auto noeud = objet_noeud->graphe.dernier_noeud_sortie;

		auto noeuds = dls::pile<Noeud *>();
		noeuds.empile(noeud);

		auto noeuds_visites = dls::ensemble<Noeud *>();

		while (!noeuds.est_vide()) {
			noeud = noeuds.haut();
			noeuds.depile();

			if (noeuds_visites.trouve(noeud) != noeuds_visites.fin()) {
				continue;
			}

			noeuds_visites.insere(noeud);

			auto operatrice = extrait_opimage(noeud->donnees());

			if (operatrice->possede_animation()) {
				ajoute_dependance(&reseau->noeud_temps, noeud_dep);
			}
			else if (operatrice->depend_sur_temps()) {
				ajoute_dependance(&reseau->noeud_temps, noeud_dep);
			}

			operatrice->renseigne_dependance(contexte, *this, noeud_dep);

			for (auto prise_entree : noeud->entrees()) {
				for (auto prise_sortie : prise_entree->liens) {
					noeuds.empile(prise_sortie->parent);
				}
			}
		}

		/* vide les tampons mémoires des noeuds déconnectés */
		for (auto n : objet_noeud->graphe.noeuds()) {
			if (noeuds_visites.trouve(n) != noeuds_visites.fin()) {
				continue;
			}

			auto operatrice = extrait_opimage(n->donnees());
			operatrice->libere_memoire();
			n->besoin_execution(true);
		}
	}

	/* Marque les graphes des objets en aval comm ayant besoin d'une exécution.
	 * À FAIRE : trouve les noeuds exacts à exécuter. */
	if (objet != nullptr) {
		auto noeud_objet = trouve_noeud_pour_objet(objet);
		marque_execution_graphe(noeud_objet);
	}
}

void CompilatriceReseau::marque_execution_temps_change()
{
	marque_execution_graphe(&reseau->noeud_temps);
}
