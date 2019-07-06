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

#include "../objet.h"
#include "../operatrice_image.h"
#include "../scene.h"

/* ************************************************************************** */

Reseau::~Reseau()
{
	reinitialise();
}

void Reseau::reinitialise()
{
	for (auto noeud : this->noeuds) {
		delete noeud;
	}

	noeuds.clear();
}

/* ************************************************************************** */

void CompilatriceReseau::cree_noeud(Objet *objet, Noeud *noeud_objet)
{
	auto iter_noeud = m_table_objet_noeud.find(objet);

	if (iter_noeud != m_table_objet_noeud.end()) {
		throw std::runtime_error("Un noeud existe déjà pour l'objet !");
	}

	auto noeud = new NoeudReseau{};
	noeud->objet = objet;
	noeud->noeud_objet = noeud_objet;
	reseau->noeuds.pousse(noeud);

	m_table_objet_noeud.insert({objet, noeud});
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud, Objet *objet)
{
	auto iter_noeud = m_table_objet_noeud.find(objet);

	if (iter_noeud == m_table_objet_noeud.end()) {
		throw std::runtime_error("Aucun noeud n'existe pour l'objet !");
	}

	ajoute_dependance(noeud, iter_noeud->second);
}

void CompilatriceReseau::ajoute_dependance(NoeudReseau *noeud_de, NoeudReseau *noeud_vers)
{
	noeud_de->entrees.insert(noeud_vers);
	noeud_vers->sorties.insert(noeud_de);
}

void CompilatriceReseau::compile_reseau(Scene *scene)
{
	reseau->reinitialise();

	/* crée les noeuds */
	for (auto paire : scene->table_objets()) {
		cree_noeud(paire.first, paire.second);
	}

	/* crée les dépendances */
	for (auto noeud_dep : reseau->noeuds) {
		auto objet = noeud_dep->objet;

		/* À FAIRE : les objets ne sont pas des 'Manipulables'. */
//		if (objet->possede_animation()) {
//			ajoute_dependance(&reseau->noeud_temps, noeud_dep);
//		}

		/* À FAIRE : n'ajoute les dépendances que pour les noeuds connectés au
		 * noeud de sortie. */

		for (auto noeud : objet->graphe.noeuds()) {
			auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

			if (operatrice->possede_animation()) {
				ajoute_dependance(&reseau->noeud_temps, noeud_dep);
			}
			else if (operatrice->depend_sur_temps()) {
				ajoute_dependance(&reseau->noeud_temps, noeud_dep);
			}

			operatrice->renseigne_dependance(*this, noeud_dep);
		}
	}
}
