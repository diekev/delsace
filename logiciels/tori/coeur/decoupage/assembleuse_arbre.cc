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
			noeud = new NoeudChaineCaractere(donnees);
			break;
		case type_noeud::VARIABLE:
			noeud = new NoeudVariable(donnees);
			break;
		case type_noeud::BLOC:
			noeud = new NoeudBloc(donnees);
			break;
		case type_noeud::SI:
			noeud = new NoeudSi(donnees);
			break;
		case type_noeud::POUR:
			noeud = new NoeudPour(donnees);
			break;
	}

	m_noeuds.pousse(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(type_noeud type, const DonneesMorceaux &donnees)
{
	auto noeud = cree_noeud(type, donnees);
	m_pile.haut()->ajoute_enfant(noeud);
}

void assembleuse_arbre::empile_noeud(type_noeud type, const DonneesMorceaux &donnees)
{
	auto noeud = cree_noeud(type, donnees);

	if (!m_pile.est_vide()) {
		m_pile.haut()->ajoute_enfant(noeud);
	}

	m_pile.empile(noeud);
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	attend_type(type);
	m_pile.depile();
}

void assembleuse_arbre::attend_type(type_noeud type)
{
	auto noeud = m_pile.haut();
	if (noeud->type() != type) {
		throw "type invalide";
	}
}

void assembleuse_arbre::imprime_arbre(std::ostream &os)
{
	if (m_pile.est_vide()) {
		os << "Arbre vide !\n";
		return;
	}

	m_pile.haut()->imprime_arbre(os, 0);
}

dls::chaine assembleuse_arbre::genere_code(tori::ObjetDictionnaire &objet) const
{
	if (m_pile.est_vide()) {
		return "";
	}

	auto tampon = dls::chaine{};
	m_pile.haut()->genere_code(tampon, objet);
	return tampon;
}
