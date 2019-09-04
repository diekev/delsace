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

#include "graphe.hh"

#include <algorithm>
#include <iostream>

#include "noeud.hh"

Graphe::Graphe(Noeud &parent)
	: noeud_parent(parent)
{}

Graphe::~Graphe()
{
	supprime_tout();
}

Noeud *Graphe::cree_noeud(const dls::chaine &nom_noeud, type_noeud type_n)
{
	auto n = memoire::loge<Noeud>("Noeud");
	n->nom = nom_noeud;
	n->type = type_n;
	n->parent = &this->noeud_parent;
	n->parent->enfants.pousse(n);
	ajoute(n);

	return n;
}

void Graphe::ajoute(Noeud *noeud)
{
	/* vérifie que le nom du noeud est unique */
	auto i = 1;
	auto nom = noeud->nom;
	auto nom_temp = nom;

	while (m_noms_noeuds.trouve(nom_temp) != m_noms_noeuds.fin()) {
		nom_temp = nom + std::to_string(i++);
	}

	noeud->nom = nom_temp;

	m_noms_noeuds.insere(nom_temp);

	m_noeuds.pousse(noeud);

	this->vide_selection();
	this->ajoute_selection(noeud);

	besoin_ajournement = true;
}

void Graphe::supprime_noeud(Noeud *noeud)
{
	this->noeud_parent.enfants.erase(
				std::find(this->noeud_parent.enfants.debut(),
						  this->noeud_parent.enfants.fin(),
						  noeud));

	memoire::deloge("Noeud", noeud);
}

void Graphe::supprime(Noeud *node)
{
	auto iter = std::find_if(m_noeuds.debut(), m_noeuds.fin(),
							 [node](Noeud *node_ptr)
	{
		return node_ptr == node;
	});

	if (iter == m_noeuds.fin()) {
		/* À FAIRE : erreur entreface */
		std::cerr << "Impossible de trouver le noeud dans le graphe !\n";
		return;
	}

	/* déconnecte entrées */
	for (PriseEntree *entree : node->entrees) {
		for (PriseSortie *sortie : entree->liens) {
			deconnecte(sortie, entree);
		}
	}

	/* déconnecte sorties */
	for (PriseSortie *sortie : node->sorties) {
		for (PriseEntree *entree : sortie->liens) {
			deconnecte(sortie, entree);
		}
	}

	supprime_noeud(*iter);

	m_noeuds.erase(iter);

	besoin_ajournement = true;
}

void Graphe::connecte(PriseSortie *sortie, PriseEntree *entree)
{
	if (!entree->liens.est_vide() && !entree->multiple_connexions) {
		std::cerr << "L'entrée est déjà connectée !\n";
		return;
	}

	/* Évite d'avoir plusieurs fois le même lien. */
	for (auto lien : entree->liens) {
		if (lien == sortie) {
			return;
		}
	}

	entree->liens.pousse(sortie);
	sortie->liens.pousse(entree);

	marque_surannee(entree->parent, nullptr);

	besoin_ajournement = true;
}

bool Graphe::deconnecte(PriseSortie *sortie, PriseEntree *entree)
{
	auto iter_entree = std::find(sortie->liens.debut(), sortie->liens.fin(), entree);

	if (iter_entree == sortie->liens.fin()) {
		std::cerr << "L'entrée n'est pas connectée à la sortie !\n";
		return false;
	}

	auto iter_sortie = std::find(entree->liens.debut(), entree->liens.fin(), sortie);

	if (iter_sortie == entree->liens.fin()) {
		std::cerr << "L'entrée n'est pas connectée à la sortie !\n";
		return false;
	}

	sortie->liens.erase(iter_entree);
	entree->liens.erase(iter_sortie);

	marque_surannee(entree->parent, nullptr);

	besoin_ajournement = true;

	return true;
}

Graphe::plage_noeud Graphe::noeuds()
{
	return plage_noeud(m_noeuds.debut(), m_noeuds.fin());
}

Graphe::plage_noeud_const Graphe::noeuds() const
{
	return plage_noeud_const(m_noeuds.debut(), m_noeuds.fin());
}

void Graphe::ajoute_selection(Noeud *noeud)
{
	m_noeuds_selectionnes.insere(noeud);
	this->noeud_actif = noeud;
}

void Graphe::vide_selection()
{
	m_noeuds_selectionnes.efface();
	this->noeud_actif = nullptr;
}

void Graphe::supprime_tout()
{
	for (auto &noeud : m_noeuds) {
		supprime_noeud(noeud);
	}

	this->noeud_parent.enfants.efface();
	m_noms_noeuds.efface();
	m_noeuds_selectionnes.efface();
	m_noeuds.efface();
}

/* ************************************************************************** */

Noeud *trouve_noeud(
		Graphe::plage_noeud noeuds,
		const float x,
		const float y)
{
	Noeud *noeud_res = nullptr;

	for (auto const &noeud : noeuds) {
		if (!noeud->rectangle.contiens(x, y)) {
			continue;
		}

		noeud_res = noeud;
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
				noeud->entrees.debut(),
				noeud->entrees.fin(),
				pred);

	if (iter != noeud->entrees.fin()) {
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
		if (!noeud->rectangle.contiens(x, y)) {
			continue;
		}

		prise_entree = trouve_prise_entree_pos(noeud, x, y);
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
				noeud->sorties.debut(),
				noeud->sorties.fin(),
				pred);

	if (iter != noeud->sorties.fin()) {
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
		if (!noeud->rectangle.contiens(x, y)) {
			continue;
		}

		prise_sortie = trouve_prise_sortie_pos(noeud, x, y);
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
		if (!noeud->rectangle.contiens(x, y)) {
			continue;
		}

		noeud_r = noeud;

		/* vérifie si oui ou non on a cliqué sur une prise d'entrée */
		prise_entree = trouve_prise_entree_pos(noeud, x, y);

		/* vérifie si oui ou non on a cliqué sur une prise de sortie */
		prise_sortie = trouve_prise_sortie_pos(noeud, x, y);
	}
}

void calcule_degree(Noeud *noeud)
{
	noeud->degree = 0;

	for (auto *prise : noeud->entrees) {
		noeud->degree += static_cast<int>(prise->liens.taille());
	}
}

void tri_topologique(Graphe &graphe)
{
	for (auto &noeud : graphe.noeuds()) {
		calcule_degree(noeud);
	}

	auto debut = graphe.noeuds().debut();
	auto fin = graphe.noeuds().fin();

	auto predicat = [](Noeud *noeud)
	{
		if (noeud->degree != 0) {
			return false;
		}

		/* Diminue le degrée des noeuds attachés à celui-ci. */
		for (auto *sortie : noeud->sorties) {
			for (auto *entree : sortie->liens) {
				entree->parent->degree -= 1;
			}
		}

		return true;
	};

	tri_topologique(debut, fin, predicat);
}
