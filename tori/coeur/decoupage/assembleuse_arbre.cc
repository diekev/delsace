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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "assembleuse_arbre.hh"

#include "arbre_syntactic.hh"

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

Noeud *assembleuse_arbre::cree_noeud(type_noeud type, const DonneesMorceaux &donnees)
{
	Noeud *noeud = nullptr;

	switch (type) {
		case type_noeud::CHAINE_CARACTERE:
			noeud = new NoeudChaineCaractere();
			break;
		case type_noeud::VARIABLE:
			noeud = new NoeudVariable();
			break;
		case type_noeud::BLOC:
			noeud = new NoeudBloc();
			break;
		case type_noeud::SI:
			noeud = new NoeudSi();
			break;
		case type_noeud::POUR:
			noeud = new NoeudPour();
			break;
	}

	m_noeuds.push_back(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(type_noeud type, const DonneesMorceaux &donnees)
{
	auto noeud = cree_noeud(type, donnees);
	m_pile.top()->ajoute_enfant(noeud);
}

void assembleuse_arbre::empile_noeud(type_noeud type, const DonneesMorceaux &donnees)
{
	auto noeud = cree_noeud(type, donnees);

	if (!m_pile.empty()) {
		m_pile.top()->ajoute_enfant(noeud);
	}

	m_pile.push(noeud);
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	auto noeud = m_pile.top();
	m_pile.pop();

	if (noeud->type() != type) {
	//	throw "type invalide";
	}
}

void assembleuse_arbre::imprime_arbre(std::ostream &os)
{
	if (m_pile.empty()) {
		os << "Arbre vide !\n";
		return;
	}

	m_pile.top()->imprime_arbre(os, 0);
}
