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

#include "graphe.h"

#include <algorithm>
#include <iostream>

void Graphe::ajoute(Noeud *noeud)
{
	/* vérifie que le nom du noeud est unique */
	auto i = 1;
	auto nom = noeud->nom();
	auto nom_temp = nom;

	while (m_noms_noeuds.find(nom_temp) != m_noms_noeuds.end()) {
		nom_temp = nom + std::to_string(i++);
	}

	noeud->nom(nom_temp);

	m_noms_noeuds.insert(nom_temp);

	m_noeuds.push_back(std::shared_ptr<Noeud>(noeud));

	this->vide_selection();
	this->ajoute_selection(noeud);

	besoin_ajournement = true;
}

void Graphe::supprime(Noeud *node)
{
	auto iter = std::find_if(m_noeuds.begin(), m_noeuds.end(),
							 [node](std::shared_ptr<Noeud> const &node_ptr)
	{
		return node_ptr.get() == node;
	});

	if (iter == m_noeuds.end()) {
		/* À FAIRE : erreur entreface */
		std::cerr << "Impossible de trouver le noeud dans le graphe !\n";
		return;
	}

	/* déconnecte entrées */
	for (PriseEntree *entree : node->entrees()) {
		if (entree->lien) {
			deconnecte(entree->lien, entree);
		}
	}

	/* déconnecte sorties */
	for (PriseSortie *sortie : node->sorties()) {
		for (PriseEntree *entree : sortie->liens) {
			deconnecte(sortie, entree);
		}
	}

	m_noeuds.erase(iter);

	besoin_ajournement = true;
}

void Graphe::connecte(PriseSortie *sortie, PriseEntree *entree)
{
	if (entree->lien != nullptr) {
		std::cerr << "L'entrée est déjà connectée !\n";
		return;
	}

	entree->lien = sortie;
	sortie->liens.push_back(entree);

	marque_surannee(entree->parent);

	besoin_ajournement = true;
}

bool Graphe::deconnecte(PriseSortie *sortie, PriseEntree *entree)
{
	auto iter = std::find(sortie->liens.begin(), sortie->liens.end(), entree);

	if (iter == sortie->liens.end()) {
		std::cerr << "L'entrée n'est pas connectée à la sortie !\n";
		return false;
	}

	sortie->liens.erase(iter);
	entree->lien = nullptr;

	marque_surannee(entree->parent);

	besoin_ajournement = true;

	return true;
}

Graphe::plage_noeud Graphe::noeuds()
{
	return plage_noeud(m_noeuds.begin(), m_noeuds.end());
}

Graphe::plage_noeud_const Graphe::noeuds() const
{
	return plage_noeud_const(m_noeuds.cbegin(), m_noeuds.cend());
}

void Graphe::ajoute_selection(Noeud *noeud)
{
	m_noeuds_selectionnes.insert(noeud);
	this->noeud_actif = noeud;
}

void Graphe::vide_selection()
{
	m_noeuds_selectionnes.clear();
	this->noeud_actif = nullptr;
}

void Graphe::supprime_tout()
{
	m_noeuds_selectionnes.clear();
	m_noeuds.clear();
}

/* ************************************************************************** */

Noeud *trouve_noeud(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y)
{
	Noeud *noeud_res = nullptr;

	for (auto const &noeud : noeuds) {
		Noeud *pointeur_noeud = noeud.get();

		if (!pointeur_noeud->rectangle().contiens(x, y)) {
			continue;
		}

		noeud_res = pointeur_noeud;
	}

	return noeud_res;
}

PriseEntree *trouve_prise_entree_pos(
		Noeud *noeud,
		const float x,
		const float y)
{
	auto pred = [&](PriseEntree *prise)
	{
		return prise->rectangle.contiens(x, y);
	};

	auto iter = std::find_if(
				noeud->entrees().begin(),
				noeud->entrees().end(),
				pred);

	if (iter != noeud->entrees().end()) {
		return *iter;
	}

	return nullptr;
}

PriseEntree *trouve_prise_entree(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y)
{
	PriseEntree *prise_entree = nullptr;

	for (auto const &noeud : noeuds) {
		Noeud *pointeur_noeud = noeud.get();

		if (!pointeur_noeud->rectangle().contiens(x, y)) {
			continue;
		}

		prise_entree = trouve_prise_entree_pos(noeud.get(), x, y);
	}

	return prise_entree;
}

PriseSortie *trouve_prise_sortie_pos(
		Noeud *noeud,
		const float x,
		const float y)
{
	auto pred = [&](PriseSortie *prise)
	{
		return prise->rectangle.contiens(x, y);
	};

	auto iter = std::find_if(
				noeud->sorties().begin(),
				noeud->sorties().end(),
				pred);

	if (iter != noeud->sorties().end()) {
		return *iter;
	}

	return nullptr;
}

PriseSortie *trouve_prise_sortie(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y)
{
	PriseSortie *prise_sortie = nullptr;


	for (auto const &noeud : noeuds) {
		Noeud *pointeur_noeud = noeud.get();

		if (!pointeur_noeud->rectangle().contiens(x, y)) {
			continue;
		}

		prise_sortie = trouve_prise_sortie_pos(noeud.get(), x, y);
	}

	return prise_sortie;
}

void trouve_noeud_prise(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y,
		Noeud *&noeud_r,
		PriseEntree *&prise_entree,
		PriseSortie *&prise_sortie)
{
	noeud_r = nullptr;
	prise_entree = nullptr;
	prise_sortie = nullptr;

	for (auto const &noeud : noeuds) {
		Noeud *pointeur_noeud = noeud.get();

		if (!pointeur_noeud->rectangle().contiens(x, y)) {
			continue;
		}

		noeud_r = pointeur_noeud;

		/* vérifie si oui ou non on a cliqué sur une prise d'entrée */
		prise_entree = trouve_prise_entree_pos(pointeur_noeud, x, y);

		/* vérifie si oui ou non on a cliqué sur une prise de sortie */
		prise_sortie = trouve_prise_sortie_pos(pointeur_noeud, x, y);
	}
}

void calcule_degree(Noeud *noeud)
{
	noeud->degre = 0;

	for (auto *prise : noeud->entrees()) {
		noeud->degre += (prise->lien != nullptr);
	}
}

template <typename I, typename P>
void tri_topologique(I debut, I fin, P predicat)
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

void tri_topologique(Graphe &graphe)
{
	for (auto &noeud : graphe.noeuds()) {
		calcule_degree(noeud.get());
	}

	auto debut = graphe.noeuds().begin();
	auto fin = graphe.noeuds().end();

	auto predicat = [](std::shared_ptr<Noeud> const &noeud)
	{
		if (noeud->degre != 0) {
			return false;
		}

		/* Diminue le degrée des noeuds attachés à celui-ci. */
		for (auto *sortie : noeud->sorties()) {
			for (auto *entree : sortie->liens) {
				entree->parent->degre -= 1;
			}
		}

		return true;
	};

	tri_topologique(debut, fin, predicat);
}
