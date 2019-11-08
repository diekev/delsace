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

#include "noeud.hh"

#include <algorithm>

#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"

#include "operatrice_image.h"
#include "operatrice_graphe_detail.hh"
#include "usine_operatrice.h"

/* ************************************************************************** */

PriseSortie::PriseSortie(dls::chaine const &noprise)
	: nom(noprise)
{}

/* ************************************************************************** */

PriseEntree::PriseEntree(dls::chaine const &noprise)
	: nom(noprise)
{}

/* ************************************************************************** */

Noeud::Noeud()
	: graphe(*this)
{}

Noeud::~Noeud()
{
	switch (type) {
		case type_noeud::OBJET:
		case type_noeud::COMPOSITE:
		case type_noeud::NUANCEUR:
		case type_noeud::RENDU:
		case type_noeud::INVALIDE:
		{
			break;
		}
		case type_noeud::OPERATRICE:
		{
			auto ptr = extrait_opimage(this->donnees);

			auto ptr_detail = dynamic_cast<OperatriceFonctionDetail *>(ptr);

			if (ptr_detail != nullptr) {
				memoire::deloge("Fonction Détail", ptr_detail);
				return;
			}

			/* Lorsque nous logeons un pointeur nous utilisons la taille de la classe
			 * dérivée pour estimer la quantité de mémoire allouée. Donc pour déloger
			 * proprement l'opératrice, en prenant en compte la taille de la classe
			 * dériviée, il faut transtyper le pointeur vers le bon type dérivée. Pour
			 * ce faire, nous devons utiliser l'usine pour avoir accès aux descriptions
			 * des opératrices. */
			auto usine = ptr->usine();
			usine->deloge(ptr);
			break;
		}
	}

	for (auto &prise : this->entrees) {
		delete prise;
	}

	for (auto &prise : this->sorties) {
		delete prise;
	}
}

float Noeud::pos_x() const
{
	return rectangle.x;
}

void Noeud::pos_x(float x)
{
	rectangle.x = x;
}

float Noeud::pos_y() const
{
	return rectangle.y;
}

void Noeud::pos_y(float y)
{
	rectangle.y = y;
}

int Noeud::hauteur() const
{
	return static_cast<int>(rectangle.hauteur);
}

void Noeud::hauteur(int h)
{
	rectangle.hauteur = static_cast<float>(h);
}

int Noeud::largeur() const
{
	return static_cast<int>(rectangle.largeur);
}

void Noeud::largeur(int l)
{
	rectangle.largeur = static_cast<float>(l);
}

void Noeud::ajoute_entree(dls::chaine const &name, const type_prise type_p, bool connexions_multiples)
{
	auto prise = new PriseEntree(name);
	prise->parent = this;
	prise->type = type_p;
	prise->multiple_connexions = connexions_multiples;

	this->entrees.pousse(prise);
}

void Noeud::ajoute_sortie(dls::chaine const &name, const type_prise type_p)
{
	auto prise = new PriseSortie(name);
	prise->parent = this;
	prise->type = type_p;

	this->sorties.pousse(prise);
}

PriseEntree *Noeud::entree(long index) const
{
	return entrees[index];
}

PriseEntree *Noeud::entree(dls::chaine const &nom_entree) const
{
	auto op = [&](PriseEntree const *prise)
	{
		return prise->nom == nom_entree;
	};

	auto iter = std::find_if(entrees.debut(), entrees.fin(), op);

	if (iter != entrees.fin()) {
		return *iter;
	}

	return nullptr;
}

PriseSortie *Noeud::sortie(long index) const
{
	return sorties[index];
}

PriseSortie *Noeud::sortie(dls::chaine const &nom_sortie) const
{
	auto op = [&](PriseSortie const *prise)
	{
		return prise->nom == nom_sortie;
	};

	auto iter = std::find_if(sorties.debut(), sorties.fin(), op);

	if (iter != sorties.fin()) {
		return *iter;
	}

	return nullptr;
}

bool Noeud::est_lie() const
{
	return a_des_entrees_liees() || a_des_sorties_liees();
}

bool Noeud::a_des_entrees_liees() const
{
	auto pred = [](PriseEntree *prise)
	{
		return !prise->liens.est_vide();
	};

	return std::any_of(entrees.debut(), entrees.fin(), pred);
}

bool Noeud::a_des_sorties_liees() const
{
	auto pred = [](PriseSortie *prise)
	{
		return !prise->liens.est_vide();
	};

	return std::any_of(sorties.debut(), sorties.fin(), pred);
}

dls::chaine Noeud::chemin() const
{
	auto res = dls::chaine();

	auto ancetres = dls::pile<Noeud const *>();
	auto p = this;

	while (p != nullptr) {
		ancetres.empile(p);
		p = p->parent;
	}

	while (!ancetres.est_vide()) {
		p = ancetres.depile();

		res += '/';
		res += p->nom;
	}

	return res;
}

/* ************************************************************************** */

void marque_surannee(Noeud *noeud, const std::function<void(Noeud *, PriseEntree *)> &rp)
{
	auto noeuds = dls::pile<Noeud *>();
	noeuds.empile(noeud);

	auto noeuds_visites = dls::ensemble<Noeud *>();

	while (!noeuds.est_vide()) {
		noeud = noeuds.depile();

		if (noeuds_visites.trouve(noeud) != noeuds_visites.fin()) {
			continue;
		}

		noeuds_visites.insere(noeud);

		noeud->besoin_execution = true;

		for (auto sortie : noeud->sorties) {
			for (auto entree : sortie->liens) {
				if (rp != nullptr) {
					rp(entree->parent, entree);
				}

				noeuds.empile(entree->parent);
			}
		}
	}
}

void marque_parent_surannee(Noeud *noeud, const std::function<void(Noeud *, PriseEntree *)> &rp)
{
	if (noeud == nullptr) {
		return;
	}

	marque_surannee(noeud, rp);
	marque_parent_surannee(noeud->parent, rp);
}

Noeud *noeud_base_hierarchie(Noeud *noeud)
{
	if (noeud == nullptr) {
		return nullptr;
	}

	while (noeud->parent != nullptr) {
		auto parent = noeud->parent;

		if (parent->graphe.type == type_graphe::RACINE_OBJET) {
			break;
		}

		if (parent->graphe.type == type_graphe::RACINE_COMPOSITE) {
			break;
		}

		noeud = parent;
	}

	return noeud;
}
