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

#include "assembleuse_arbre.h"

#include <llvm/IR/Module.h>

#include "arbre_syntactic.h"

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

Noeud *assembleuse_arbre::ajoute_noeud(int type, const std::string &chaine, int id, bool ajoute)
{
	auto noeud = cree_noeud(type, chaine, id);

	if (!m_pile.empty() && ajoute) {
		m_pile.top()->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(Noeud *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

Noeud *assembleuse_arbre::cree_noeud(int type, const std::string &chaine, int id)
{
	Noeud *noeud = nullptr;

	switch (type) {
		case NOEUD_RACINE:
			noeud = new NoeudRacine(chaine, id);
			break;
		case NOEUD_APPEL_FONCTION:
			noeud = new NoeudAppelFonction(chaine, id);
			break;
		case NOEUD_DECLARATION_FONCTION:
			noeud = new NoeudDeclarationFonction(chaine, id);
			break;
		case NOEUD_EXPRESSION:
			noeud = new NoeudExpression(chaine, id);
			break;
		case NOEUD_ASSIGNATION_VARIABLE:
			noeud = new NoeudAssignationVariable(chaine, id);
			break;
		case NOEUD_VARIABLE:
			noeud = new NoeudVariable(chaine, id);
			break;
		case NOEUD_NOMBRE_ENTIER:
			noeud = new NoeudNombreEntier(chaine, id);
			break;
		case NOEUD_NOMBRE_REEL:
			noeud = new NoeudNombreReel(chaine, id);
			break;
		case NOEUD_OPERATION:
			noeud = new NoeudOperation(chaine, id);
			break;
		case NOEUD_RETOUR:
			noeud = new NoeudRetour(chaine, id);
			break;
	}

	if (noeud != nullptr) {
		m_noeuds.push_back(noeud);
	}

	return noeud;
}

void assembleuse_arbre::sors_noeud(int type)
{
	m_pile.pop();
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	m_pile.top()->imprime_code(os, 0);
}

void assembleuse_arbre::genere_code_llvm()
{
	auto contexte = llvm::LLVMContext();
	auto module = new llvm::Module("top", contexte);

	m_pile.top()->genere_code_llvm(contexte, module);

	module->dump();
	delete module;
}
